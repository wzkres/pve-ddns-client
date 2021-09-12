#include "public_ip_getter_ipify.h"

#include "glog/logging.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://api6.ipify.org/?format=json";
static const char * API_HOST_V4 = "https://api.ipify.org/?format=json";

const std::string & PublicIpGetterIpify::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterIpify::setCredentials(const std::string & cred_str)
{
    if (!cred_str.empty())
        LOG(WARNING) << "Credential is not needed for ipify public IP getter!";
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
        LOG(WARNING) << "'" << v6_ip << "' is not valid IPv6 ip!";
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
        LOG(WARNING) << "Failed to request '" << api_host << "', response code is " << resp_code << ", response is "
                     << resp_data << "!";
        return "";
    }
    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        LOG(WARNING) << "Failed to parse response json, error '" << rapidjson::GetParseError_En(ok.Code())
                     << "' (" << ok.Offset() << ")";
        return "";
    }

    if (d.HasMember("ip") && d["ip"].IsString())
    {
        LOG(INFO) << "Successfully got my ip: " << d["ip"].GetString() << " from '" << api_host << "'.";
        return d["ip"].GetString();
    }

    return "";
}
