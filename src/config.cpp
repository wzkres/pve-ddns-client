#include "config.h"

#include "glog/logging.h"
#include "yaml-cpp/yaml.h"

// Parse general config from yaml node
static void parse_general_config(const YAML::Node & yaml_node, Config & config)
{
    if (yaml_node["update-interval-ms"])
    {
        const auto interval_ms = yaml_node["update-interval-ms"].as<uint64_t>();
        config._update_interval = std::chrono::milliseconds(interval_ms);
    }
    if (yaml_node["log-overdue-days"])
        config._log_overdue_days = yaml_node["log-overdue-days"].as<int>();
    if (yaml_node["public-ip"])
    {
        const auto & pi = yaml_node["public-ip"];
        if (pi["service"])
            config._public_ip_service = pi["service"].as<std::string>();
        if (pi["credentials"])
            config._public_ip_credentials = pi["credentials"].as<std::string>();
    }
    if (yaml_node["pve-api"])
    {
        const auto & pa = yaml_node["pve-api"];
        if (pa["host"])
        {
            config._pve_api_host = pa["host"].as<std::string>();
            // Append trailing slash if needed
            if (config._pve_api_host[config._pve_api_host.length() - 1] != '/')
                config._pve_api_host.append("/");
        }
        if (pa["user"])
            config._pve_api_user = pa["user"].as<std::string>();
        if (pa["realm"])
            config._pve_api_realm = pa["realm"].as<std::string>();
        if (pa["token-id"])
            config._pve_api_token_id = pa["token-id"].as<std::string>();
        if (pa["token-uuid"])
            config._pve_api_token_uuid = pa["token-uuid"].as<std::string>();
    }
    if (yaml_node["sync_host_static_v6_address"])
    {
        const auto val = yaml_node["sync_host_static_v6_address"].as<std::string>();
        config._sync_host_static_v6_address = val == "true";
    }
}

// Parse ddns config from yaml node
static void parse_ddns_config(const YAML::Node & yaml_node, config_node & cfg_node)
{
    if (yaml_node["node"])
        cfg_node.node = yaml_node["node"].as<std::string>();
    if (yaml_node["iface"])
        cfg_node.iface = yaml_node["iface"].as<std::string>();
    if (yaml_node["dns"])
        cfg_node.dns_type = yaml_node["dns"].as<std::string>();
    if (yaml_node["api-key"])
        cfg_node.api_key = yaml_node["api-key"].as<std::string>();
    if (yaml_node["api-secret"])
        cfg_node.api_secret = yaml_node["api-secret"].as<std::string>();
    if (yaml_node["ipv4"] && yaml_node["ipv4"].IsSequence())
    {
        const auto & ipv4_domains = yaml_node["ipv4"];
        for (auto it = ipv4_domains.begin(); it != ipv4_domains.end(); ++it)
            cfg_node.ipv4_domains.emplace_back(it->as<std::string>());
    }
    if (yaml_node["ipv6"] && yaml_node["ipv6"].IsSequence())
    {
        const auto & ipv4_domains = yaml_node["ipv6"];
        for (auto it = ipv4_domains.begin(); it != ipv4_domains.end(); ++it)
            cfg_node.ipv6_domains.emplace_back(it->as<std::string>());
    }
}

bool Config::loadConfig(const std::string & config_file)
{
    bool conf_valid = false;
    try
    {
        YAML::Node conf = YAML::LoadFile(config_file);
        // Reset current loaded configs first
        _host_config = {};
        _guest_configs.clear();
        // Mandatory general node
        if (conf["general"])
        {
            LOG(INFO) << "Found general config!";
            parse_general_config(conf["general"], *this);
        }
        else
        {
            LOG(WARNING) << "General config not found!";
            return conf_valid;
        }
        // Load host config if specified
        if (conf["host"])
        {
            LOG(INFO) << "Found host config!";
            parse_ddns_config(conf["host"], _host_config);
            LOG(INFO) << "Host config loaded, " << _host_config.ipv4_domains.size() << " ipv4 domain(s), "
                << _host_config.ipv6_domains.size() << " ipv6 domain(s)!";
        }
        // Load each guest config if any
        if (conf["guests"] && conf["guests"].IsSequence())
        {
            LOG(INFO) << "Found guests config!";
            const auto & guests = conf["guests"];
            for (auto it = guests.begin(); it != guests.end(); ++it)
            {
                const auto & guest_node = it->as<YAML::Node>();
                if (guest_node["vmid"])
                {
                    config_node temp = {};
                    parse_ddns_config(guest_node, temp);
                    _guest_configs.emplace(guest_node["vmid"].as<int>(), temp);
                }
            }
            LOG(INFO) << _guest_configs.size() << " guest config(s) loaded!";
        }

        conf_valid = true;
    }
    catch (...)
    {
        LOG(ERROR) << "Failed to load config yaml file '" << config_file << "'!";
    }

    return conf_valid;
}
