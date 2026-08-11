#include "dns_service_dnspod.h"

#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://dnsapi.cn/";
static const char * API_VERSION = "Info.Version";
static const char * API_RECORD_LIST = "Record.List";
static const char * API_RECORD_DDNS = "Record.Ddns";

const std::string & DnsServiceDnspod::getServiceName()
{
    return _service_name;
}

bool DnsServiceDnspod::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        SPDLOG_WARN("Credentials string is empty!");
        return false;
    }
    std::string::size_type comma_pos = cred_str.find(',');
    if (std::string::npos == comma_pos)
    {
        SPDLOG_WARN("Invalid credentials string '{}', should be in format 'TOKEN_ID,TOKEN'!", cred_str);
        return false;
    }
    _token = cred_str;

    std::string api_version;
    if (!getVersion(api_version))
    {
        SPDLOG_WARN("Failed to get API version, maybe wrong token!");
        return false;
    }

    SPDLOG_INFO("Successfully got API version '{}'.", api_version);
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
        SPDLOG_WARN("Failed to request '{}', response code is {}, response is {}!", req_url, resp_code, resp_data);
        return false;
    }

    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        SPDLOG_WARN("Failed to parse response json, error '{}' ({})", 
            rapidjson::GetParseError_En(ok.Code()), ok.Offset());
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

    SPDLOG_WARN("Invalid response '{}'!", resp_data);
    return false;
}

std::string DnsServiceDnspod::getIp(const std::string & domain, bool is_v4)
{
    if (domain.empty())
    {
        SPDLOG_WARN("Invalid param!");
        return "";
    }

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
        SPDLOG_WARN("Failed to request '{}', response code is {}, response is {}!", req_url, resp_code, resp_data);
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
                    std::string record_id, line_id;
                    if (r.HasMember("id") && r["id"].IsString())
                        record_id = r["id"].GetString();
                    if (r.HasMember("line_id") && r["line_id"].IsString())
                        line_id = r["line_id"].GetString();
                    if (!updateRecordCache(domain, is_v4, record_id, line_id))
                    {
                        SPDLOG_WARN("Failed to update record cache for IP{} domain '{}'!", 
                            (is_v4 ? "v4" : "v6"), domain);
                        return "";
                    }
                    if (r.HasMember("value") && r["value"].IsString())
                        return r["value"].GetString();
                }
            }
        }
    }

    SPDLOG_WARN("Invalid response '{}'!", resp_data);
    return "";
}

bool DnsServiceDnspod::setIp(const std::string & domain, const std::string & ip, bool is_v4)
{
    if (domain.empty() || ip.empty())
    {
        SPDLOG_WARN("Invalid params, domain '{}', ip '{}'!", domain, ip);
        return false;
    }

    const auto & record_cache = getRecordCache(domain, is_v4);
    if (nullptr == record_cache)
    {
        SPDLOG_WARN("No record cache found for IP{} domain '{}'!", (is_v4 ? "v4" : "v6"), domain);
        return false;
    }

    const auto & config = Config::getInstance();

    const auto sub_domain = get_sub_domain(domain);
    const std::string req_url = fmt::format("{}{}", API_HOST, API_RECORD_DDNS);
    const std::string req_body = fmt::format(
        R"(login_token={}&domain={}&sub_domain={}&record_id={}&record_line_id={}&format=json&lang=en)",
        _token, sub_domain.first, sub_domain.second, record_cache->record_id, record_cache->line_id
    );

    int resp_code = 0;
    std::string resp_data;
    const bool ret = http_req(req_url, req_body, config._http_timeout_ms, {}, resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        SPDLOG_WARN("Failed to request '{}', response code is {}, response is {}!", req_url, resp_code, resp_data);
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

    if (d.HasMember("status") && d["status"].IsObject())
    {
        const auto & status = d["status"];
        if (status.HasMember("code") && status["code"].IsString() && str_iequals(status["code"].GetString(), "1"))
            return true;
    }

    SPDLOG_WARN("Invalid response '{}'!", resp_data);

    return false;
}

bool DnsServiceDnspod::updateRecordCache(const std::string & domain, bool is_v4,
                                         const std::string & record_id, const std::string & line_id)
{
    if (domain.empty() || record_id.empty() || line_id.empty())
    {
        SPDLOG_WARN("Invalid params, domain '{}', record id '{}', line id '{}'", domain, record_id, line_id);
        return false;
    }

    auto found = std::find_if(_records_cache.begin(), _records_cache.end(),
                              [&domain, is_v4](const dnspod_record_cache & rc)
    {
        return rc.domain == domain && rc.is_v4 == is_v4;
    });

    if (found != _records_cache.end())
    {
        // Update exist record cache
        found->record_id = record_id;
        found->line_id = line_id;
    }
    else
    {
        // Build up new record cache
        _records_cache.emplace_back(dnspod_record_cache{ domain, is_v4, record_id, line_id });
    }

    return true;
}

const dnspod_record_cache * DnsServiceDnspod::getRecordCache(const std::string & domain, bool is_v4) const
{
    if (domain.empty())
    {
        SPDLOG_WARN("Invalid param!");
        return nullptr;
    }

    auto found = std::find_if(_records_cache.begin(), _records_cache.end(),
                              [&domain, is_v4](const dnspod_record_cache & rc)
    {
        return rc.domain == domain && rc.is_v4 == is_v4;
    });

    if (found != _records_cache.end())
        return &(*found);

    return nullptr;
}
