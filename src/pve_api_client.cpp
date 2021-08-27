#include "pve_api_client.h"

#include "glog/logging.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "config.h"
#include "utils.h"

static const char * API_VERSION = "api2/json/version";
static const char * API_HOST_NETWORK = "api2/json/nodes/{}/network/{}";

static std::string get_pve_api_http_auth_header()
{
    const auto & config = Config::getInstance();
//    Authorization: PVEAPIToken=USER@REALM!TOKENID=UUID
    return fmt::format("Authorization: PVEAPIToken={}@{}!{}={}",
                       config._pve_api_user, config._pve_api_realm,
                       config._pve_api_token_id, config._pve_api_token_uuid);
}

bool PveApiClient::init()
{
    const bool ret = checkApiHost();
    if (!ret)
        LOG(WARNING) << "Failed to checkApiHost!";
    return ret;
}

bool PveApiClient::req(const std::string & api_url, const std::string & req_data,
                       int & resp_code, std::string & resp_data) const
{
    std::vector<std::string> headers = { get_pve_api_http_auth_header() };
    return http_req(api_url, req_data, Config::getInstance()._http_timeout_ms, headers, resp_code, resp_data);
}

bool PveApiClient::checkApiHost() const
{
    const auto & config = Config::getInstance();

    const std::string req_url = fmt::format("{}{}", config._pve_api_host, API_VERSION);
    int resp_code = 0;
    std::string resp_data;
    const bool ret = req(req_url, "", resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << "!";
        return ret;
    }
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        LOG(WARNING) << "Failed to parse response json, error '" << rapidjson::GetParseError_En(ok.Code())
            << "' (" << ok.Offset() << ")";
        return false;
    }

    if (d.HasMember("data") && d["data"].IsObject())
    {
        const auto & data = d["data"];
        if (data.HasMember("version") && data["version"].IsString())
        {
            LOG(INFO) << "Successfully got API version: " << data["version"].GetString();
            return true;
        }
    }

    return false;
}
