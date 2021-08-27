#include "dns_service_porkbun.h"

#include "fmt/format.h"
#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://porkbun.com/api/json/v3/";
static const char * API_RETRIEVE = "dns/retrieveByNameType/{}/{}/{}";

const std::string & DnsServicePorkbun::getServiceName()
{
    return _service_name;
}

bool DnsServicePorkbun::setCredentials(const std::string & cred_str)
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

std::string DnsServicePorkbun::getIpv4(const std::string & domain)
{
    return getIp(domain, true);
}

std::string DnsServicePorkbun::getIpv6(const std::string & domain)
{
    return getIp(domain, false);
}

bool DnsServicePorkbun::setIpv4(const std::string & domain)
{
    return false;
}

bool DnsServicePorkbun::setIpv6(const std::string & domain)
{
    return false;
}

std::string DnsServicePorkbun::getIp(const std::string & domain, bool is_v4)
{
    const auto sub_domain = get_sub_domain(domain);
    const std::string api_part = fmt::format(API_RETRIEVE, sub_domain.first, is_v4 ? "A" : "AAAA", sub_domain.second);
    const std::string req_url = fmt::format("{}{}", API_HOST, api_part);
    const std::string req_body = fmt::format(R"({{"secretapikey":"{}","apikey":"{}"}})", _api_secret, _api_key);

    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(req_url, req_body, Config::getInstance()._http_timeout_ms, {}, resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << "!";
        return "";
    }
//    LOG(INFO) << resp_data;
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
            auto it = d["records"].Begin();
            while (it != d["records"].End())
                return (*it)["content"].GetString();
        }
    }

    return "";
}
