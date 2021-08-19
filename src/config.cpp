#include "config.h"

#include "glog/logging.h"
#include "yaml-cpp/yaml.h"

static void fill_config_node(const YAML::Node & yaml_node, config_node & cfg_node)
{
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
        // Load host config if specified
        if (conf["host"])
        {
            LOG(INFO) << "Found host conf!";
            fill_config_node(conf["host"], _host_config);
            LOG(INFO) << "Host conf loaded, " << _host_config.ipv4_domains.size() << " ipv4 domain(s), "
                << _host_config.ipv6_domains.size() << " ipv6 domain(s)!";
        }
        // Load each guest config if any
        if (conf["guests"] && conf["guests"].IsSequence())
        {
            LOG(INFO) << "Found guests conf!";
            const auto & guests = conf["guests"];
            for (auto it = guests.begin(); it != guests.end(); ++it)
            {
                const auto & guest_node = it->as<YAML::Node>();
                if (guest_node["vmid"])
                {
                    config_node temp = {};
                    fill_config_node(guest_node, temp);
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
