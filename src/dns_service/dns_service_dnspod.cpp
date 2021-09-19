#include "dns_service_dnspod.h"

#include "fmt/format.h"
#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://dnsapi.cn/";
static const char * API_VERSION = "Info.Version";
static const char * API_RECORD_LIST = "Record.List";
static const char * API_RECORD_DDNS= "Record.Ddns";

const std::string & DnsServiceDnspod::getServiceName()
{
    return _service_name;
}

bool DnsServiceDnspod::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        LOG(WARNING) << "Credentials string is empty!";
        return false;
    }
    std::string::size_type comma_pos = cred_str.find(',');
    if (std::string::npos == comma_pos)
    {
        LOG(WARNING) << "Invalid credentials string '" << cred_str << "', should be in format 'TOKEN_ID,TOKEN'!";
        return false;
    }
    _token = cred_str;

    std::string api_version;
    if (!getVersion(api_version))
    {
        LOG(WARNING) << "Failed to get API version, maybe wrong token!";
        return false;
    }

    LOG(INFO) << "Successfully got API version '" << api_version << "'.";
    return true;
}

std::string DnsServiceDnspod::getIpv4(const std::string & domain)
{
    return getIp(domain, true);
}

std::string DnsServiceDnspod::getIpv6(const std::string & domain)
{
    return getIp(domain, false);
}

bool DnsServiceDnspod::setIpv4(const std::string & domain, const std::string & ip)
{
    return setIp(domain, ip, true);
}

bool DnsServiceDnspod::setIpv6(const std::string & domain, const std::string & ip)
{
    return setIp(domain, ip, false);
}

bool DnsServiceDnspod::getVersion(std::string & version)
{
    const auto & config = Config::getInstance();

    const std::string req_url = fmt::format("{}{}", API_HOST, API_VERSION);
    const std::string req_body = fmt::format(R"(login_token={}&format=json)", _token);

    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(req_url, req_body, config._http_timeout_ms, {}, resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << ", response is "
                     << resp_data << "!";
        return false;
    }

    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        LOG(WARNING) << "Failed to parse response json, error '" << rapidjson::GetParseError_En(ok.Code())
                     << "' (" << ok.Offset() << ")";
        return false;
    }

    if (d.HasMember("status") && d["status"].IsObject())
    {
        const auto & status = d["status"];
        if (status.HasMember("code") && status["code"].IsString() && str_iequals(status["code"].GetString(), "1"))
        {
            if (status.HasMember("message") && status["message"].IsString())
            {
                version = status["message"].GetString();
                return true;
            }
        }
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return false;
}

std::string DnsServiceDnspod::getIp(const std::string & domain, bool is_v4)
{
    const auto & config = Config::getInstance();

    const auto sub_domain = get_sub_domain(domain);
    const std::string req_url = fmt::format("{}{}", API_HOST, API_RECORD_LIST);
    const std::string req_body = fmt::format(
        R"(login_token={}&domain={}&sub_domain={}&record_type={}&format=json&lang=en)",
        _token, sub_domain.first, sub_domain.second, is_v4 ? "A" : "AAAA"
    );

    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(req_url, req_body, config._http_timeout_ms, {}, resp_code, resp_data);
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

    if (d.HasMember("status") && d["status"].IsObject())
    {
        const auto & status = d["status"];
        if (status.HasMember("code") && status["code"].IsString() && str_iequals(status["code"].GetString(), "1"))
        {
            if (d.HasMember("records") && d["records"].IsArray())
            {
                const auto & result = d["records"].GetArray();
                for (const auto & r : result)
                {
                    if (r.HasMember("value") && r["value"].IsString())
                        return r["value"].GetString();
                }
            }
        }
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return "";
}

bool DnsServiceDnspod::setIp(const std::string & domain, const std::string & ip, bool is_v4)
{
    LOG(WARNING) << "Not implemented!";
    return false;
}
