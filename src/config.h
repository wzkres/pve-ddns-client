#ifndef PVE_DDNS_CLIENT_SRC_CONFIG_H
#define PVE_DDNS_CLIENT_SRC_CONFIG_H

#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>

// Config node
typedef struct config_node_
{
    // PVE node
    std::string node;
    // Interface name
    std::string iface;
    // DNS service type
    std::string dns_type;
    // Credentials
    std::string credentials;
    // IPv4 domain names to update
    std::vector<std::string> ipv4_domains;
    // IPv6 domain names to update
    std::vector<std::string> ipv6_domains;
} config_node;

// DNS record node
typedef struct dns_record_node_
{
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

    // Default http request timeout 30s
    long _http_timeout_ms = 30000;

    // Update interval
    std::chrono::milliseconds _update_interval = std::chrono::milliseconds(300000);
    // Log cleaner keep days
    int _log_overdue_days = 3;
    // Default realtime logging output
    int _log_buf_secs = 0;
    // Max size in MB per log file
    int _max_log_size_mb = 2;
    // Long-running service mode
    bool _service_mode = true;

    // Public IP service related
    std::string _public_ip_service;
    std::string _public_ip_credentials;

    // PVE API related stuff
    std::string _pve_api_host;
    std::string _pve_api_user;
    std::string _pve_api_realm;
    std::string _pve_api_token_id;
    std::string _pve_api_token_uuid;

    bool _sync_host_static_v6_address = false;

    // Client config
    config_node  _client_config;
    // Host config
    config_node _host_config;
    // Guest configs
    std::unordered_map<int, config_node> _guest_configs;

    std::string _my_public_ipv4;
    std::string _my_public_ipv6;

    std::unordered_map<std::string, dns_record_node> _ipv4_records;
    std::unordered_map<std::string, dns_record_node> _ipv6_records;

    // Last update time
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
