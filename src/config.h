#ifndef PVE_DDNS_CLIENT_SRC_CONFIG_H
#define PVE_DDNS_CLIENT_SRC_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>

// Config node
typedef struct _config_node
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

// Global config singleton
class Config
{
public:
    static Config & getInstance()
    {
        static Config instance;
        return instance;
    }

    bool loadConfig(const std::string & config_file);

    // Host config
    config_node _host_config;
    // Guest configs
    std::unordered_map<int, config_node> _guest_configs;

private:
    // ctor is hidden
    Config() = default;
    // copy ctor is hidden
    Config(Config const &) = default;
    // assign op is hidden
    Config & operator=(Config const &) = default;
};

#endif //PVE_DDNS_CLIENT_SRC_CONFIG_H
