#include <memory>
#include <thread>
#include <unordered_map>

#include "fmt/format.h"
#include "glog/logging.h"
#include "curl/curl.h"
#include "cmdline.h"

#include "config.h"
#include "utils.h"
#include "public_ip/public_ip_getter.h"
#include "dns_service/dns_service.h"
#include "pve_api_client.h"

// Running flag
static bool g_running = true;
static IPublicIpGetter * g_ip_getter = nullptr;
static std::unordered_map<std::string, IDnsService *> g_dns_services;

// Command line params handling
static bool parse_cmd(int argc, char * argv[])
{
    if (argc < 1 || nullptr == argv || nullptr == argv[0])
    {
        LOG(ERROR) << "Invalid command line params!";
        return false;
    }

    cmdline::parser p;
    p.add("version", 'v', "Print version");
    p.add("help", 'h', "Show usage");
    p.add<std::string>("config", 'c', "Config yaml file to load", false, "./pve-ddns-client.yml");
    p.add<std::string>("log", 'l', "Log file path", false, "./");

    const auto show_usage = [&p]()
    {
      const std::string usage = p.usage();
      std::cout << usage << std::endl;
    };

    bool ret = p.parse(argc, argv);
    if (ret)
    {
        if (p.exist("version"))
        {
            std::cout << "Ver 0.0.1" << std::endl;
            ret = false;
        }
        else if (p.exist("help"))
        {
            ret = false;
            show_usage();
        }
        else
        {
            bool args_valid = false;
            do
            {
                auto & config = Config::getInstance();

                config._yml_path = p.get<std::string>("config");
                if (config._yml_path.empty()) break;

                config._log_path = p.get<std::string>("log");
                if (config._log_path.empty()) break;

                args_valid = true;
            } while (false);

            if (!args_valid)
            {
                ret = false;
                std::cerr << "Invalid params!" << std::endl;
                show_usage();
            }
        }
    }
    else
    {
        std::cerr << "Failed to parse command line params!" << std::endl;
        show_usage();
    }

    return ret;
}

static bool init_public_ip_getter()
{
    if (nullptr != g_ip_getter)
    {
        LOG(WARNING) << "g_ip_getter is not nullptr!";
        return false;
    }

    auto & cfg = Config::getInstance();
    g_ip_getter = PublicIpGetterFactory::create(cfg._public_ip_service);
    if (nullptr == g_ip_getter)
    {
        LOG(WARNING) << "Failed to create public ip getter " << PUBLIC_IP_GETTER_PORKBUN << "!";
        return false;
    }
    if (!g_ip_getter->setCredentials(cfg._public_ip_credentials))
    {
        LOG(WARNING) << "Failed to setCredentials!";
        PublicIpGetterFactory::destroy(g_ip_getter);
        return false;
    }

    cfg._my_public_ipv4 = g_ip_getter->getIpv4();
    cfg._my_public_ipv6 = g_ip_getter->getIpv6();

    return true;
}

static void cleanup_public_ip_getter()
{
    if (nullptr != g_ip_getter)
        PublicIpGetterFactory::destroy(g_ip_getter);
}

static IDnsService * get_dns_service(const std::string & service_key)
{
    auto it = g_dns_services.find(service_key);
    if (g_dns_services.end() == it)
        return nullptr;
    return it->second;
}

static bool create_dns_service(const std::string & dns_type,
                               const std::string & api_key,
                               const std::string & api_secret)
{
    const std::string key = get_dns_service_key(dns_type, api_key, api_secret);
    if (g_dns_services.find(key) == g_dns_services.end())
    {
        IDnsService * dns_service = DnsServiceFactory::create(dns_type);
        if (nullptr == dns_service)
        {
            LOG(WARNING) << "Failed to create dns service " << dns_type << "!";
            return false;
        }
        if (!dns_service->setCredentials(fmt::format("{},{}", api_key, api_secret)))
        {
            LOG(WARNING) << "Failed to setCredentials!";
            DnsServiceFactory::destroy(dns_service);
            return false;
        }
        g_dns_services.emplace(key, dns_service);
    }

    return true;
}

static void cleanup_dns_services()
{
    for (auto & kv : g_dns_services)
        DnsServiceFactory::destroy(kv.second);
    g_dns_services.clear();
}


static bool init_dns_services()
{
    const auto & cfg = Config::getInstance();
    if (!cfg._host_config.dns_type.empty() && !cfg._host_config.api_key.empty() && !cfg._host_config.api_secret.empty())
        if (!create_dns_service(cfg._host_config.dns_type, cfg._host_config.api_key, cfg._host_config.api_secret))
            return false;

    for (auto & guest_config : cfg._guest_configs)
    {
        if (!guest_config.second.dns_type.empty() &&
            !guest_config.second.api_key.empty() && !guest_config.second.api_secret.empty())
        {
            if (!create_dns_service(guest_config.second.dns_type,
                                    guest_config.second.api_key,
                                    guest_config.second.api_secret))
            {
                cleanup_dns_services();
                return false;
            }
        }
    }

    return true;
}

static void init_dns_records()
{
    auto & cfg = Config::getInstance();
    std::string dns_service_key = get_dns_service_key(cfg._host_config.dns_type,
                                                      cfg._host_config.api_key,
                                                      cfg._host_config.api_secret);
    auto * host_dns_service = get_dns_service(dns_service_key);
    if (nullptr != host_dns_service)
    {
        for (const auto & domain : cfg._host_config.ipv4_domains)
        {
            auto found = cfg._ipv4_records.find(domain);
            if (cfg._ipv4_records.end() == found)
            {
                std::string ip = host_dns_service->getIpv4(domain);
                LOG(INFO) << "Domain '" << domain << "', A record is: '" << ip << "'.";
                cfg._ipv4_records.emplace(
                    domain,
                    dns_record_node
                    {
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ),
                        ip
                    }
                );
            }
        }

        for (const auto & domain : cfg._host_config.ipv6_domains)
        {
            auto found = cfg._ipv6_records.find(domain);
            if (cfg._ipv6_records.end() == found)
            {
                std::string ip = host_dns_service->getIpv6(domain);
                LOG(INFO) << "Domain '" << domain << "', AAAA record is: '" << ip << "'.";
                cfg._ipv6_records.emplace(
                    domain,
                    dns_record_node
                    {
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ),
                        ip
                    }
                );
            }
        }
    }

    for (auto & guest : cfg._guest_configs)
    {
        const auto & dns_config = guest.second;
        dns_service_key = get_dns_service_key(dns_config.dns_type, dns_config.api_key, dns_config.api_secret);
        auto * guest_dns_service = get_dns_service(dns_service_key);
        if (nullptr != guest_dns_service)
        {
            for (const auto & domain : dns_config.ipv4_domains)
            {
                auto found = cfg._ipv4_records.find(domain);
                if (cfg._ipv4_records.end() == found)
                {
                    std::string ip = guest_dns_service->getIpv4(domain);
                    LOG(INFO) << "Domain '" << domain << "', A record is: '" << ip << "'.";
                    cfg._ipv4_records.emplace(
                        domain,
                        dns_record_node
                        {
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()
                            ),
                            ip
                        }
                    );
                }
            }

            for (const auto & domain : dns_config.ipv6_domains)
            {
                auto found = cfg._ipv6_records.find(domain);
                if (cfg._ipv6_records.end() == found)
                {
                    std::string ip = guest_dns_service->getIpv6(domain);
                    LOG(INFO) << "Domain '" << domain << "', AAAA record is: '" << ip << "'.";
                    cfg._ipv6_records.emplace(
                        domain,
                        dns_record_node
                        {
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()
                            ),
                            ip
                        }
                    );
                }
            }
        }
    }
}

// main
int main(int argc, char * argv[])
{
    if (!parse_cmd(argc, argv))
        return EXIT_SUCCESS;

    Config & cfg = Config::getInstance();

    FLAGS_log_dir = cfg._log_path;
    FLAGS_alsologtostderr = true;
    google::SetLogFilenameExtension(".log");
#ifndef NDEBUG
    google::SetLogDestination(google::GLOG_INFO, "");
#endif
    // Only one log file
    google::SetLogDestination(google::GLOG_ERROR, "");
    google::SetLogDestination(google::GLOG_FATAL, "");
    google::SetLogDestination(google::GLOG_WARNING, "");
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    curl_global_init(CURL_GLOBAL_ALL);

    LOG(INFO) << "Starting up, loading config from '" << cfg._yml_path << "'...";

    const bool cfg_valid = cfg.loadConfig(cfg._yml_path);
    if (cfg_valid)
    {
        LOG(INFO) << "Config loaded!";
        google::EnableLogCleaner(cfg._log_overdue_days);

        do
        {
            if (!init_public_ip_getter())
            {
                LOG(WARNING) << "Failed to init public ip!";
                break;
            }
            LOG(INFO) << "Public IP getter inited!";
            if (!init_dns_services())
            {
                LOG(WARNING) << "Failed to init dns services!";
                break;
            }
            LOG(INFO) << "All DNS services inited!";
            init_dns_records();
            LOG(INFO) << "Initial dns records updated!";

            std::shared_ptr<PveApiClient> pve_api_client = std::make_shared<PveApiClient>();
            if (pve_api_client->init())
                LOG(INFO) << "PVE API client inited!";
            else
            {
                LOG(WARNING) << "PVE API client failed to init!";
                break;
            }

            int counter = 0;
            // Service loop
            while (g_running)
            {
                auto elasped_time = std::chrono::system_clock::now().time_since_epoch() - cfg._last_update_time;
                if (elasped_time > cfg._update_interval)
                {
                    cfg._last_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    );

                    ++counter;
                    if (counter > 5)
                        g_running = false;
                }
                else
                    std::this_thread::sleep_for(cfg._update_interval - elasped_time);
            }
        } while (false);
    }
    else
        LOG(WARNING) << "Failed to load config from '" << cfg._yml_path << "'!";

    LOG(INFO) << "Shutting down...";
    cleanup_dns_services();
    cleanup_public_ip_getter();
    curl_global_cleanup();
    google::ShutdownGoogleLogging();

    return EXIT_SUCCESS;
}
