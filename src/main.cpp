#include "glog/logging.h"
#define CURL_STATICLIB
#include "curl/curl.h"
#include "cmdline.h"

#include "config.h"

// Command line params handling
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

// main
int main(int argc, char * argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    curl_global_init(CURL_GLOBAL_ALL);

    std::string conf_yaml;
    if (!parse_cmd(argc, argv, conf_yaml))
        return EXIT_SUCCESS;

    LOG(INFO) << "Starting up, loading config yaml from '" << conf_yaml << "'...";
    Config & cfg = Config::getInstance();
    const bool cfg_valid = cfg.loadConfig(conf_yaml);
    if (cfg_valid)
        LOG(INFO) << "Config loaded!";

    LOG(INFO) << "Shutting down...";
    curl_global_cleanup();
    google::ShutdownGoogleLogging();
    return EXIT_SUCCESS;
}
