#include "public_ip_getter_ipify.h"

#include "spdlog/spdlog.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static constexpr const char * API_HOST = "https://api6.ipify.org/?format=json";
static constexpr const char * API_HOST_V4 = "https://api.ipify.org/?format=json";

const std::string & PublicIpGetterIpify::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterIpify::setCredentials(const std::string & cred_str)
{
    if (!cred_str.empty())
        SPDLOG_WARN("Credential is not needed for ipify public IP getter!");
    return true;
}

std::string PublicIpGetterIpify::getIpv4()
{
    return getIp(API_HOST_V4);
}

std::string PublicIpGetterIpify::getIpv6()
{
    std::string v6_ip = getIp(API_HOST);
    if (!is_ipv6(v6_ip))
    {
        SPDLOG_WARN("'{}' is not valid IPv6 ip!", v6_ip);
        return "";
    }
    return v6_ip;
}

std::string PublicIpGetterIpify::getIp(const std::string & api_host)
{
    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(api_host, "", Config::getInstance()._http_timeout_ms, {}, resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        SPDLOG_WARN("Failed to request '{}', response code is {}, response is {}!", api_host, resp_code, resp_data);
        return "";
    }
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        SPDLOG_WARN("Failed to parse response json, error '{}' ({})", 
            rapidjson::GetParseError_En(ok.Code()), ok.Offset());
        return "";
    }

    if (d.HasMember("ip") && d["ip"].IsString())
    {
        SPDLOG_TRACE("Successfully got my ip: {} from '{}'.", d["ip"].GetString(), api_host);
        return d["ip"].GetString();
    }

    return "";
}
