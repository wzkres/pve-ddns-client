#include "public_ip_getter_porkbun.h"

#include "glog/logging.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://porkbun.com/api/json/v3/";
static const char * API_HOST_V4 = "https://api-ipv4.porkbun.com/api/json/v3/";
static const char * API_PING = "ping";

const std::string & PublicIpGetterPorkbun::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterPorkbun::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        LOG(WARNING) << "Credentials string is empty!";
        return false;
    }
    std::string::size_type comma_pos = cred_str.find(',');
    if (std::string::npos == comma_pos)
    {
        LOG(WARNING) << "Invalid credentials string '" << cred_str << "', should be in format 'API_KEY,API_SECRET'!";
        return false;
    }
    _api_key = cred_str.substr(0, comma_pos);
    _api_secret = cred_str.substr(comma_pos + 1);

    return true;
}

std::string PublicIpGetterPorkbun::getIpv4()
{
    return getIp(API_HOST_V4);
}

std::string PublicIpGetterPorkbun::getIpv6()
{
    std::string v6_ip = getIp(API_HOST);
    if (!is_ipv6(v6_ip))
    {
        LOG(WARNING) << "'" << v6_ip << "' is not valid IPv6 ip!";
        return "";
    }
    return v6_ip;
}

std::string PublicIpGetterPorkbun::getIp(const std::string & api_host) const
{
    const std::string req_url = fmt::format("{}{}", api_host, API_PING);
    const std::string req_body = fmt::format(R"({{"secretapikey":"{}","apikey":"{}"}})", _api_secret, _api_key);
    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(req_url, req_body, Config::getInstance()._http_timeout_ms, {}, resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << ", response is "
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

    if (d.HasMember("status") && d["status"].IsString())
    {
        const std::string status_str = d["status"].GetString();
        if ("SUCCESS" == status_str)
        {
            if (d.HasMember("yourIp") && d["yourIp"].IsString())
            {
                LOG(INFO) << "Successfully got my ip: " << d["yourIp"].GetString() << " from '" << api_host << "'.";
                return d["yourIp"].GetString();
            }
        }
    }

    return "";
}
