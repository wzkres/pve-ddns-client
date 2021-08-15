#include <vector>
#include <unordered_map>

#include "glog/logging.h"
#include "cmdline.h"
#include "yaml-cpp/yaml.h"

typedef struct _config_node
{
    std::string dns_type;
    std::string api_key;
    std::string api_secret;
    std::vector<std::string> ipv4_domains;
    std::vector<std::string> ipv6_domains;
} config_node;

typedef struct _pve_ddns_config
{
    config_node host_node;
    std::unordered_map<int, config_node> guest_nodes;
} pve_ddns_config;

static bool parse_cmd(int argc, char * argv[], std::string & out_conf_yml)
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
                out_conf_yml = p.get<std::string>("config");
                if (out_conf_yml.empty()) break;

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

static bool load_conf(const std::string & conf_yaml)
{
    bool conf_valid = false;
    try
    {
        YAML::Node conf = YAML::LoadFile(conf_yaml);
        if (conf["host"])
        {
            LOG(INFO) << "Found host conf!";
            const auto & host_conf = conf["host"];
        }
        conf_valid = true;
    }
    catch (...)
    {
        LOG(ERROR) << "Failed to load config yaml file '" << conf_yaml << "'!";
    }

    return conf_valid;
}

// main
int main(int argc, char * argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    std::string conf_yaml;

    if (!parse_cmd(argc, argv, conf_yaml))
        return EXIT_SUCCESS;

    LOG(INFO) << "Starting up, loading config yaml from '" << conf_yaml << "'...";
    const bool conf_valid = load_conf(conf_yaml);
    if (conf_valid)
    {
        LOG(INFO) << "Config loaded!";
    }

    LOG(INFO) << "Shutting down...";
    google::ShutdownGoogleLogging();
    return EXIT_SUCCESS;
}
