#ifndef PVE_DDNS_CLIENT_SRC_CONFIG_H
#define PVE_DDNS_CLIENT_SRC_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>

// Config node
typedef struct config_node_
{
    // DNS service type
    std::string dns_type;
    // API key
    std::string api_key;
    // API secret
    std::string api_secret;
    // IPv4 domain names to update
    std::vector<std::string> ipv4_domains;
    // IPv6 domain names to update
    std::vector<std::string> ipv6_domains;
} config_node;

// DNS record node
typedef struct dns_record_node_
{
    // Domain name
    std::string domain_name;
    // Is IPv6
    bool is_v6;
    // Last get time (resolve)
    std::chrono::milliseconds last_get_time;
    // Last IP
    std::string last_ip;
} dns_record_node;

// Global config singleton
class Config
{
public:
    static Config & getInstance()
    {
        static Config instance;
        return instance;
    }

    // Load config yaml
    bool loadConfig(const std::string & config_file);

    // Config yaml file path
    std::string _yml_path;
    // Log file saving path
    std::string _log_path;

    // Update interval
    std::chrono::milliseconds _update_interval = std::chrono::milliseconds(300000);
    // Log cleaner keep days
    int _log_overdue_days = 3;
    // PVE API related stuff
    std::string _pve_api_host;
    std::string _pve_api_node;
    std::string _pve_api_iface;
    std::string _pve_api_user;
    std::string _pve_api_realm;
    std::string _pve_api_token_id;
    std::string _pve_api_token_uuid;

    // Host config
    config_node _host_config;
    // Guest configs
    std::unordered_map<int, config_node> _guest_configs;

    std::chrono::milliseconds _last_update_time = std::chrono::milliseconds(0);

private:
    // ctor is hidden
    Config() = default;
    // copy ctor is hidden
    Config(Config const &) = default;
    // assign op is hidden
    Config & operator=(Config const &) = default;
};

#endif //PVE_DDNS_CLIENT_SRC_CONFIG_H
