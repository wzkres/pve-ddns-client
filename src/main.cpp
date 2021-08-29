#include <memory>
#include <thread>
#include <unordered_map>

//#ifdef WIN32
//#include <windows.h>
//#else
//#include <signal.h>
//#endif

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
static volatile bool g_running = true;
static IPublicIpGetter * g_ip_getter = nullptr;
static std::unordered_map<std::string, IDnsService *> g_dns_services;

#ifdef WIN32
static BOOL WINAPI ctrl_handler(DWORD fdw_ctrl_type)
{
    if (CTRL_C_EVENT == fdw_ctrl_type)
    {
        LOG(INFO) << "Received ctrl+c event, stopping...";
        g_running = false;
        return TRUE;
    }

    LOG(WARNING) << "Received ctrl event: " << fdw_ctrl_type << ", ignored!";
    return FALSE;
}
#else
#endif

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

static bool update_dns_records(const config_node & config_node, const std::string & ip, const bool is_v4)
{
    auto & cfg = Config::getInstance();
    std::string dns_service_key = get_dns_service_key(cfg._host_config.dns_type,
                                                      cfg._host_config.api_key,
                                                      cfg._host_config.api_secret);
    auto * dns_service = get_dns_service(dns_service_key);
    if (nullptr == dns_service)
    {
        LOG(WARNING) << "Failed to find dns service of '" << cfg._host_config.dns_type << "'!";
        return false;
    }

    if (is_v4)
    {
        for (const auto & domain : config_node.ipv4_domains)
        {
            auto found = cfg._ipv4_records.find(domain);
            if (cfg._ipv4_records.end() == found)
            {
                LOG(WARNING) << "IPv4 domain '" << domain << "' dns record not found!";
                return false;
            }
            if (found->second.last_ip != ip)
            {
                LOG(INFO) << "IPv4 domain '" << domain << "' dns record address changed from '" << found->second.last_ip
                    << "' to '" << ip << "', updating...";
                if (dns_service->setIpv4(domain, ip))
                {
                    LOG(INFO) << "IPv4 record of domain '" << domain << "' successfully updated from '"
                        << found->second.last_ip << "' to '" << ip << "'.";
                    found->second.last_ip = ip;
                    found->second.last_get_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    );
                }
                else
                {
                    LOG(WARNING) << "Failed to update IPv4 record from '" << found->second.last_ip << "' to '"
                        << ip << "' of domain '" << domain << "'!";
                }
            }
            else
                LOG(INFO) << "IPv4 domain '" << domain << "' dns record address '" << ip << "' not changed.";
        }
    }
    else
    {
        for (const auto & domain : config_node.ipv6_domains)
        {
            auto found = cfg._ipv6_records.find(domain);
            if (cfg._ipv6_records.end() == found)
            {
                LOG(WARNING) << "IPv6 domain '" << domain << "' dns record not found!";
                return false;
            }
            if (found->second.last_ip != ip)
            {
                LOG(INFO) << "IPv6 domain '" << domain << "' dns record address changed from '" << found->second.last_ip
                << "' to '" << ip << "', updating...";
                if (dns_service->setIpv6(domain, ip))
                {
                    LOG(INFO) << "IPv6 record of domain '" << domain << "' successfully updated from '"
                        << found->second.last_ip << "' to '" << ip << "'.";
                    found->second.last_ip = ip;
                    found->second.last_get_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    );
                }
                else
                {
                    LOG(WARNING) << "Failed to update IPv6 record from '" << found->second.last_ip << "' to '"
                        << ip << "' of domain '" << domain << "'!";
                }
            }
            else
                LOG(INFO) << "IPv6 domain '" << domain << "' dns record address '" << ip << "' not changed.";
        }
    }

    return true;
}

static bool sync_host_static_v6_address(const std::shared_ptr<PveApiClient> & pve_api_client,
                                        const std::string & host_v6_addr, const std::string & guest_v6_addr)
{
    if (host_v6_addr.empty() || guest_v6_addr.empty())
        return true;

    int counter = 1;
    auto host_4th_colon_pos = host_v6_addr.find(':');
    while (host_4th_colon_pos != std::string::npos && counter < 4)
    {
        host_4th_colon_pos = host_v6_addr.find(':', host_4th_colon_pos + 1);
        ++counter;
    }
    if (counter != 4)
    {
        LOG(WARNING) << "Invalid host v6 address '" << host_v6_addr << "'!";
        return false;
    }

    const std::string host_1st_part = host_v6_addr.substr(0, host_4th_colon_pos);
    const std::string host_2nd_part = host_v6_addr.substr(host_4th_colon_pos + 1);

    counter = 1;
    auto guest_4th_colon_pos = guest_v6_addr.find(':');
    while (guest_4th_colon_pos != std::string::npos && counter < 4)
    {
        guest_4th_colon_pos = guest_v6_addr.find(':', guest_4th_colon_pos + 1);
        ++counter;
    }
    if (counter != 4)
    {
        LOG(WARNING) << "Invalid guest v6 address '" << guest_v6_addr << "'!";
        return false;
    }

    const std::string guest_1st_part = guest_v6_addr.substr(0, guest_4th_colon_pos);
    if (host_1st_part == guest_1st_part)
    {
        LOG(INFO) << "No need to sync host v6 static address, 1st part '" << host_1st_part << "' not changed.";
        return true;
    }

    const std::string new_host_v6_address = fmt::format("{}:{}", guest_1st_part, host_2nd_part);
    LOG(INFO) << "Host v6 static address 1st part changed from '" << host_1st_part << "' to '"
        << guest_1st_part << "', updating host static IPv6 address to '" << new_host_v6_address << "'...";

    int retry_count = 0;
    while(retry_count < 5)
    {
        const auto & cfg = Config::getInstance();
        if (!pve_api_client->setHostIpv6Address(cfg._host_config.node, cfg._host_config.iface, new_host_v6_address))
        {
            LOG(WARNING) << "Failed to update synced host static IPv6 address, retry in 1 minute("
                << retry_count << ")...";
            std::this_thread::sleep_for(std::chrono::minutes(1));
            ++retry_count;
            continue;
        }
        else
            LOG(INFO) << "Host static IPv6 address successfully updated.";

        std::this_thread::sleep_for(std::chrono::seconds(10));

        if (!update_dns_records(cfg._host_config, new_host_v6_address, false))
        {
            LOG(WARNING) << "Failed to update synced host v6 dns records, retry in 1 minute("
                << retry_count << ")";
            std::this_thread::sleep_for(std::chrono::minutes(1));
            ++retry_count;
            continue;
        }
        else
            LOG(INFO) << "Synced host v6 dns records successfully updated!";

        break;
    }

    return true;
}

// main
int main(int argc, char * argv[])
{
//#ifdef WIN32
//    SetConsoleCtrlHandler(ctrl_handler, TRUE);
//#endif
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

            // Service loop
            while (g_running)
            {
                auto elasped_time = std::chrono::system_clock::now().time_since_epoch() - cfg._last_update_time;
                if (elasped_time > cfg._update_interval)
                {
                    cfg._last_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    );

                    auto ret = pve_api_client->getHostIp(cfg._host_config.node, cfg._host_config.iface);
                    const std::string host_v6_addr = ret.second;
                    if (!cfg._host_config.ipv4_domains.empty() && ret.first.empty())
                    {
                        LOG(WARNING) << "Failed to get host IPv4 address!";
                        g_running = false;
                    }
                    else if (!cfg._host_config.ipv4_domains.empty() && !ret.first.empty())
                    {
                        if (!update_dns_records(cfg._host_config, ret.first, true))
                            LOG(WARNING) << "Failed to update host v4 dns records!";
                    }

                    if (!cfg._host_config.ipv6_domains.empty() && ret.second.empty())
                    {
                        LOG(WARNING) << "Failed to get host IPv6 address!";
                        g_running = false;
                    }
                    else if (!cfg._host_config.ipv6_domains.empty() && !ret.second.empty())
                    {
                        if (!update_dns_records(cfg._host_config, ret.second, false))
                            LOG(WARNING) << "Failed to update host v6 dns records!";
                    }

                    std::string guest_v6_addr;
                    for (auto & guest : cfg._guest_configs)
                    {
                        ret = pve_api_client->getGuestIp(guest.second.node, guest.first, guest.second.iface);
                        if (!ret.second.empty() && guest_v6_addr.empty())
                            guest_v6_addr = ret.second;
                        if (!guest.second.ipv4_domains.empty() && ret.first.empty())
                        {
                            LOG(WARNING) << "Failed to get guest(vmid: " << guest.first << ") IPv4 address!";
                            g_running = false;
                        }
                        else if (!guest.second.ipv4_domains.empty() && !ret.first.empty())
                        {
                            if (!update_dns_records(guest.second, ret.first, true))
                                LOG(WARNING) << "Failed to update guest(vmid: " << guest.first << ") v4 dns records!";
                        }

                        if (!guest.second.ipv6_domains.empty() && ret.second.empty())
                        {
                            LOG(WARNING) << "Failed to get guest(vmid: " << guest.first << ") IPv6 address!";
                            g_running = false;
                        }
                        else if (!guest.second.ipv6_domains.empty() && !ret.second.empty())
                        {
                            if (!update_dns_records(guest.second, ret.second, false))
                                LOG(WARNING) << "Failed to update guest(vmid: " << guest.first << ") v6 dns records!";
                        }
                    }

                    if (cfg._sync_host_static_v6_address)
                        if (!sync_host_static_v6_address(pve_api_client, host_v6_addr, guest_v6_addr))
                            LOG(WARNING) << "Failed to sync host static IPv6 address!";
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
