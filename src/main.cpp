#include <memory>

#include "glog/logging.h"
#include "curl/curl.h"
#include "cmdline.h"

#include "config.h"
#include "pve_api_client.h"

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

                config.yml_path = p.get<std::string>("config");
                if (config.yml_path.empty()) break;

                config.log_path = p.get<std::string>("log");
                if (config.log_path.empty()) break;

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

    FLAGS_log_dir = cfg.log_path;
    FLAGS_alsologtostderr = true;
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    curl_global_init(CURL_GLOBAL_ALL);

    LOG(INFO) << "Starting up, loading config from '" << cfg.yml_path << "'...";

    const bool cfg_valid = cfg.loadConfig(cfg.yml_path);
    if (cfg_valid)
    {
        LOG(INFO) << "Config loaded!";
        google::EnableLogCleaner(cfg.log_overdue_days);
        std::shared_ptr<PveApiClient> pve_api_client = std::make_shared<PveApiClient>();
        if (pve_api_client->init())
            LOG(INFO) << "PVE API client inited!";
        else
            LOG(WARNING) << "PVE API client failed to init!";
    }
    else
        LOG(WARNING) << "Failed to load config from '" << cfg.yml_path << "'!";

    LOG(INFO) << "Shutting down...";
    curl_global_cleanup();
    google::ShutdownGoogleLogging();
    return EXIT_SUCCESS;
}
