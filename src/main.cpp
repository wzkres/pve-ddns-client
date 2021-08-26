#include <memory>
#include <thread>
#include <unordered_map>

#include "fmt/format.h"
#include "glog/logging.h"
#include "curl/curl.h"
#include "cmdline.h"

#include "config.h"
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
            g_ip_getter = PublicIpGetterFactory::create(cfg._public_ip_service);
            if (nullptr == g_ip_getter)
            {
                LOG(WARNING) << "Failed to create public ip getter " << PUBLIC_IP_GETTER_PORKBUN << "!";
                break;
            }
            if (!g_ip_getter->setCredentials(cfg._public_ip_credentials))
            {
                LOG(WARNING) << "Failed to setCredentials!";
                PublicIpGetterFactory::destroy(g_ip_getter);
                break;
            }

            if (!cfg._host_config.dns_type.empty() &&
                !cfg._host_config.api_key.empty() &&
                !cfg._host_config.api_secret.empty())
            {
                const std::string dns_service_key = fmt::format("{}:{}:{}",
                                                                cfg._host_config.dns_type,
                                                                cfg._host_config.api_key,
                                                                cfg._host_config.api_secret);
                if (g_dns_services.find(dns_service_key) == g_dns_services.end())
                {
                    IDnsService * dns_service = DnsServiceFactory::create(cfg._host_config.dns_type);
                    if (nullptr == dns_service)
                    {
                        LOG(WARNING) << "Failed to create dns service " << cfg._host_config.dns_type << "!";
                        break;
                    }
                    if (!dns_service->setCredentials(fmt::format("{},{}",
                                                                 cfg._host_config.api_key,
                                                                 cfg._host_config.api_secret)))
                    {
                        LOG(WARNING) << "Failed to setCredentials!";
                        DnsServiceFactory::destroy(dns_service);
                        break;
                    }
                    g_dns_services.emplace(dns_service_key, dns_service);
                    dns_service->getIpv4("test");
                }
            }

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

                    cfg._my_public_ipv4 = g_ip_getter->getIpv4();
                    cfg._my_public_ipv6 = g_ip_getter->getIpv6();

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
    for (auto & kv : g_dns_services)
        DnsServiceFactory::destroy(kv.second);
    g_dns_services.clear();

    if (nullptr != g_ip_getter)
        PublicIpGetterFactory::destroy(g_ip_getter);
    curl_global_cleanup();
    google::ShutdownGoogleLogging();
    return EXIT_SUCCESS;
}
