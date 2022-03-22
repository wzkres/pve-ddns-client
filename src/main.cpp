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
#include "pve/pve_api_client.h"
#include "pve/pve_pct_wrapper.h"

// Main loop running flag
static volatile bool g_running = true;
// Public IP getter service instance
static std::shared_ptr<IPublicIpGetter> g_ip_getter;
// DNS service instances
static std::shared_ptr<std::unordered_map<size_t, IDnsService *>> g_dns_services;

//#ifdef WIN32
//static BOOL WINAPI ctrl_handler(DWORD fdw_ctrl_type)
//{
//    if (CTRL_C_EVENT == fdw_ctrl_type)
//    {
//        LOG(INFO) << "Received ctrl+c event, stopping...";
//        g_running = false;
//        return TRUE;
//    }
//
//    LOG(WARNING) << "Received ctrl event: " << fdw_ctrl_type << ", ignored!";
//    return FALSE;
//}
//#else
//#endif

// Command line params handling
static bool parse_cmd(int argc, char * argv[])
{
    if (argc < 1 || nullptr == argv || nullptr == argv[0])
    {
        std::cerr << "Invalid command line params!" << std::endl;
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
            std::cout << "Ver " << get_version_string() << std::endl;
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
    auto * ip_getter = PublicIpGetterFactory::create(cfg._public_ip_service);
    if (nullptr == ip_getter)
    {
        LOG(WARNING) << "Failed to create public ip getter " << cfg._public_ip_service << "!";
        return false;
    }
    g_ip_getter = std::shared_ptr<IPublicIpGetter>(ip_getter, [](IPublicIpGetter * ip_getter)
    {
        if (nullptr == ip_getter)
            return;
        PublicIpGetterFactory::destroy(ip_getter);
    });
    if (!g_ip_getter->setCredentials(cfg._public_ip_credentials))
    {
        LOG(WARNING) << "Failed to setCredentials to ip getter " << cfg._public_ip_service << "!";
        g_ip_getter.reset();
        return false;
    }
    // Initial retrieval of public IPv4 and IPv6 addresses;
    cfg._my_public_ipv4 = g_ip_getter->getIpv4();
    cfg._my_public_ipv6 = g_ip_getter->getIpv6();

    return true;
}

static void cleanup_public_ip_getter()
{
    g_ip_getter.reset();
}

static IDnsService * get_dns_service(const size_t service_key)
{
    if (nullptr == g_dns_services)
    {
        LOG(WARNING) << "Invalid g_dns_services!";
        return nullptr;
    }

    auto it = g_dns_services->find(service_key);
    if (g_dns_services->end() == it)
        return nullptr;
    return it->second;
}

static bool create_dns_service(const std::string & dns_type, const std::string & credentials)
{
    if (nullptr == g_dns_services)
    {
        LOG(WARNING) << "Invalid g_dns_services!";
        return false;
    }

    const size_t key = get_dns_service_key(dns_type, credentials);
    if (g_dns_services->find(key) == g_dns_services->end())
    {
        IDnsService * dns_service = DnsServiceFactory::create(dns_type);
        if (nullptr == dns_service)
        {
            LOG(WARNING) << "Failed to create dns service " << dns_type << "!";
            return false;
        }
        if (!dns_service->setCredentials(credentials))
        {
            LOG(WARNING) << "Failed to setCredentials!";
            DnsServiceFactory::destroy(dns_service);
            return false;
        }
        g_dns_services->emplace(key, dns_service);
    }

    return true;
}

static void cleanup_dns_services()
{
    if (nullptr == g_dns_services)
    {
        LOG(WARNING) << "Invalid g_dns_services!";
        return;
    }

    for (auto & kv : *g_dns_services)
        DnsServiceFactory::destroy(kv.second);
    g_dns_services->clear();
}

static bool init_dns_services()
{
    const auto & cfg = Config::getInstance();
    if (!cfg._client_config.dns_type.empty() && !cfg._client_config.credentials.empty())
        if (!create_dns_service(cfg._client_config.dns_type, cfg._client_config.credentials))
            return false;

    if (!cfg._host_config.dns_type.empty() && !cfg._host_config.credentials.empty())
        if (!create_dns_service(cfg._host_config.dns_type, cfg._host_config.credentials))
            return false;

    for (auto & guest_config : cfg._guest_configs)
    {
        if (!guest_config.second.dns_type.empty() && !guest_config.second.credentials.empty())
        {
            if (!create_dns_service(guest_config.second.dns_type, guest_config.second.credentials))
            {
                cleanup_dns_services();
                return false;
            }
        }
    }

    return true;
}

static void init_node_dns_records(const config_node & node)
{
    Config & cfg = Config::getInstance();
    const size_t dns_service_key = get_dns_service_key(node.dns_type, node.credentials);
    auto * dns_service = get_dns_service(dns_service_key);
    if (nullptr != dns_service)
    {
        for (const auto & domain : node.ipv4_domains)
        {
            auto found = cfg._ipv4_records.find(domain);
            if (cfg._ipv4_records.end() == found)
            {
                std::string ip = dns_service->getIpv4(domain);
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

        for (const auto & domain : node.ipv6_domains)
        {
            auto found = cfg._ipv6_records.find(domain);
            if (cfg._ipv6_records.end() == found)
            {
                std::string ip = dns_service->getIpv6(domain);
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

static void init_dns_records()
{
    auto & cfg = Config::getInstance();
    init_node_dns_records(cfg._client_config);
    init_node_dns_records(cfg._host_config);
    for (auto & guest : cfg._guest_configs)
    {
        const auto & dns_config = guest.second;
        init_node_dns_records(guest.second);
    }
}

static bool update_dns_records_v4(const config_node & config_node, const std::string & ip, IDnsService * dns_service)
{
    if (ip.empty() || nullptr == dns_service)
    {
        LOG(WARNING) << "Invalid params!";
        return false;
    }
    auto & cfg = Config::getInstance();
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
                LOG(WARNING) << "Failed to update IPv4 record from '" << found->second.last_ip << "' to '"
                             << ip << "' of domain '" << domain << "'!";
        }
        else
            LOG(INFO) << "IPv4 domain '" << domain << "' dns record address '" << ip << "' not changed.";
    }
    return true;
}

static bool update_dns_records_v6(const config_node & config_node, const std::string & ip, IDnsService * dns_service)
{
    if (ip.empty() || nullptr == dns_service)
    {
        LOG(WARNING) << "Invalid params!";
        return false;
    }
    auto & cfg = Config::getInstance();
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
                LOG(WARNING) << "Failed to update IPv6 record from '" << found->second.last_ip << "' to '"
                             << ip << "' of domain '" << domain << "'!";
        }
        else
            LOG(INFO) << "IPv6 domain '" << domain << "' dns record address '" << ip << "' not changed.";
    }
    return true;
}

static bool update_dns_records(const config_node & config_node, const std::string & ip, const bool is_v4)
{
    auto & cfg = Config::getInstance();
    size_t dns_service_key = get_dns_service_key(config_node.dns_type, config_node.credentials);
    auto * dns_service = get_dns_service(dns_service_key);
    if (nullptr == dns_service)
    {
        LOG(WARNING) << "Failed to find dns service of '" << config_node.dns_type << "'!";
        return false;
    }

    if (is_v4)
    {
        if (!update_dns_records_v4(config_node, ip, dns_service))
            return false;
    }
    else
    {
        if (!update_dns_records_v6(config_node, ip, dns_service))
            return false;
    }

    return true;
}

static bool sync_host_static_v6_address(const std::shared_ptr<PveApiClient> & pve_api_client,
                                        const std::string & host_v4_addr, const std::string & host_v6_addr,
                                        const std::string & guest_v6_addr)
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
        if (!pve_api_client->setHostNetworkAddress(cfg._host_config.node, cfg._host_config.iface,
                                                   host_v4_addr, new_host_v6_address))
        {
            LOG(WARNING) << "Failed to update synced host static IPv6 address, retry in 1 minute("
                         << retry_count << ")...";

            if (!pve_api_client->revertHostNetworkChange(cfg._host_config.node))
                LOG(WARNING) << "Failed to revert host network change!";
            else
                LOG(INFO) << "Host network change successfully reverted.";

            std::this_thread::sleep_for(std::chrono::minutes(1));
            ++retry_count;
            continue;
        }

        LOG(INFO) << "Host static IPv6 address successfully updated, applying change...";
        if (!pve_api_client->applyHostNetworkChange(cfg._host_config.node))
        {
            LOG(WARNING) << "Failed to apply host network change, retry in 1 minute("
                << retry_count << ")...";

            if (!pve_api_client->revertHostNetworkChange(cfg._host_config.node))
                LOG(WARNING) << "Failed to revert host network change!";
            else
                LOG(INFO) << "Host network change successfully reverted.";

            std::this_thread::sleep_for(std::chrono::minutes(1));
            ++retry_count;
            continue;
        }
        LOG(INFO) << "Host network change successfully applied!";

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

static bool initialize(int argc, char * argv[])
{
//#ifdef WIN32
//    SetConsoleCtrlHandler(ctrl_handler, TRUE);
//#endif
    if (!parse_cmd(argc, argv))
        return false;

    Config & cfg = Config::getInstance();
    const bool cfg_valid = cfg.loadConfig(cfg._yml_path);
    if (!cfg_valid)
    {
        std::cerr << "Failed to load config from '" << cfg._yml_path << "'!" << std::endl;
        return false;
    }

    FLAGS_log_dir = cfg._log_path;
    FLAGS_alsologtostderr = true;
    fLI::FLAGS_max_log_size = cfg._max_log_size_mb;
    fLI::FLAGS_logbufsecs = cfg._log_buf_secs;

    google::SetLogFilenameExtension(".log");
#ifndef NDEBUG
    google::SetLogDestination(google::GLOG_INFO, "");
#endif
    // Only one log file
    google::SetLogDestination(google::GLOG_ERROR, "");
    google::SetLogDestination(google::GLOG_FATAL, "");
    google::SetLogDestination(google::GLOG_WARNING, "");
    google::EnableLogCleaner(cfg._log_overdue_days);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    curl_global_init(CURL_GLOBAL_ALL);

    return true;
}

static bool initialize_services(std::shared_ptr<PveApiClient> & pve_api_client,
                                std::shared_ptr<PvePctWrapper> & pve_pct_wrapper)
{
    g_dns_services = std::make_shared<std::unordered_map<size_t, IDnsService *>>();
    if (!init_public_ip_getter())
    {
        LOG(WARNING) << "Failed to init public ip!";
        return false;
    }
    LOG(INFO) << "Public IP getter inited!";
    if (!init_dns_services())
    {
        LOG(WARNING) << "Failed to init dns services!";
        return false;
    }
    LOG(INFO) << "All DNS services inited!";
    init_dns_records();
    LOG(INFO) << "Initial dns records updated!";

    const Config & cfg = Config::getInstance();
    // Only initialize PVE related stuff if needed
    if (!cfg._host_config.ipv4_domains.empty() || !cfg._host_config.ipv6_domains.empty() ||
        !cfg._guest_configs.empty())
    {
        pve_api_client = std::make_shared<PveApiClient>();
        if (nullptr == pve_api_client)
        {
            LOG(ERROR) << "Failed to allocate PveApiClient!";
            return false;
        }
        if (pve_api_client->init())
            LOG(INFO) << "PVE API client inited!";
        else
        {
            LOG(WARNING) << "PVE API client failed to init, but host and/or guest(s) node config present!";
            return false;
        }

        pve_pct_wrapper = std::make_shared<PvePctWrapper>();
        if (nullptr == pve_pct_wrapper)
        {
            LOG(ERROR) << "Failed to allocate PvePctWrapper!";
            return false;
        }
        if (pve_pct_wrapper->init())
            LOG(INFO) << "PVE pct wrapper inited!";
        else
            LOG(WARNING) << "PVE pct wrapper failed to init, DDNS updating of LXC guests will not work!";
    }

    return true;
}

static void update_client()
{
    Config & cfg = Config::getInstance();
    if (!cfg._client_config.ipv4_domains.empty())
    {
        cfg._my_public_ipv4 = g_ip_getter->getIpv4();
        if (cfg._my_public_ipv4.empty())
            LOG(WARNING) << "Failed to get client public IPv4 address!";
        else if (!update_dns_records(cfg._client_config, cfg._my_public_ipv4, true))
            LOG(WARNING) << "Failed to update client v4 dns records!";
    }

    if (!cfg._client_config.ipv6_domains.empty())
    {
        cfg._my_public_ipv6 = g_ip_getter->getIpv6();
        if (cfg._my_public_ipv6.empty())
            LOG(WARNING) << "Failed to get client public IPv6 address!";
        else if (!update_dns_records(cfg._client_config, cfg._my_public_ipv6, false))
            LOG(WARNING) << "Failed to update client v6 dns record!";
    }
}

static void update_host(const std::shared_ptr<PveApiClient> & pve_api_client,
                        std::string & host_v4_addr, std::string & host_v6_addr)
{
    if (nullptr == pve_api_client)
    {
        LOG(WARNING) << "Invalid pve_api_client!";
        return;
    }
    const Config & cfg = Config::getInstance();
    if (!cfg._host_config.ipv4_domains.empty() || !cfg._host_config.ipv6_domains.empty())
    {
        auto ret = pve_api_client->getHostIp(cfg._host_config.node, cfg._host_config.iface);
        host_v4_addr = ret.first;
        host_v6_addr = ret.second;
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
    }
}

static void update_guests(const std::shared_ptr<PveApiClient> & pve_api_client,
                          const std::shared_ptr<PvePctWrapper> & pve_pct_wrapper,
                          const std::string & host_v4_addr, const std::string & host_v6_addr)
{
    if (nullptr == pve_api_client || nullptr == pve_pct_wrapper)
    {
        LOG(WARNING) << "Invalid pve_api_client and/or pve_pct_wrapper!";
        return;
    }
    std::string guest_v6_addr;
    const Config & cfg = Config::getInstance();
    for (auto & guest : cfg._guest_configs)
    {
        std::pair<std::string, std::string> ret = {};
        if (pve_pct_wrapper->isLxcGuest(guest.first))
            ret = pve_pct_wrapper->getGuestIp(guest.first, guest.second.iface);
        else
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

    if ((!cfg._host_config.ipv4_domains.empty() || !cfg._host_config.ipv6_domains.empty()) &&
        cfg._sync_host_static_v6_address)
        if (!sync_host_static_v6_address(pve_api_client, host_v4_addr, host_v6_addr, guest_v6_addr))
            LOG(WARNING) << "Failed to sync host static IPv6 address!";
}

// main
int main(int argc, char * argv[])
{
    if (!initialize(argc, argv))
        return EXIT_SUCCESS;

    Config & cfg = Config::getInstance();
    LOG(INFO) << "Starting up, ver " << get_version_string() << ", config loaded from '" << cfg._yml_path << "'.";
    LOG(INFO) << (cfg._service_mode ? "Running" : "Not running") << " in service mode...";

    do
    {
        std::shared_ptr<PveApiClient> pve_api_client;
        std::shared_ptr<PvePctWrapper> pve_pct_wrapper;
        if (!initialize_services(pve_api_client, pve_pct_wrapper))
        {
            LOG(WARNING) << "Failed to initialize_services!";
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
                update_client();
                std::string host_v4_addr, host_v6_addr;
                update_host(pve_api_client, host_v4_addr, host_v6_addr);
                update_guests(pve_api_client, pve_pct_wrapper, host_v4_addr, host_v6_addr);
            }
            else if (!cfg._service_mode)
                break;
            else
                std::this_thread::sleep_for(cfg._update_interval - elasped_time);
        }
    } while (false);

    LOG(INFO) << "Shutting down...";
    cleanup_dns_services();
    cleanup_public_ip_getter();
    curl_global_cleanup();
    google::ShutdownGoogleLogging();

    return EXIT_SUCCESS;
}
