#include "dns_service_cloudflare.h"

#include "fmt/format.h"
#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "../utils.h"
#include "../config.h"

static const char * API_HOST = "https://api.cloudflare.com/client/v4/";
static const char * API_VERIFY_TOKEN = "user/tokens/verify";
static const char * API_LIST_ZONES = "zones";
static const char * API_LIST_RECORDS = "zones/{}/dns_records";
static const char * API_PATCH_RECORD = "zones/{}/dns_records/{}";

const std::string & DnsServiceCloudflare::getServiceName()
{
    return _service_name;
}

bool DnsServiceCloudflare::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        LOG(WARNING) << "Credentials string is empty!";
        return false;
    }
    _token = cred_str;
    if (!verifyToken())
    {
        LOG(WARNING) << "Invalid cloudflare API token '" << cred_str << "'!";
        return false;
    }

    return true;
}

std::string DnsServiceCloudflare::getIpv4(const std::string & domain)
{
    return getIp(domain, true);
}

std::string DnsServiceCloudflare::getIpv6(const std::string & domain)
{
    return getIp(domain, false);
}

bool DnsServiceCloudflare::setIpv4(const std::string & domain, const std::string & ip)
{
    return setIp(domain, ip, true);
}

bool DnsServiceCloudflare::setIpv6(const std::string & domain, const std::string & ip)
{
    return setIp(domain, ip, false);
}

bool DnsServiceCloudflare::verifyToken()
{
    if (_token.empty())
    {
        LOG(WARNING) << "Empty token string!";
        return false;
    }

    const auto & config = Config::getInstance();

    const std::string req_url = fmt::format("{}{}", API_HOST, API_VERIFY_TOKEN);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { fmt::format("Authorization: Bearer {}", _token) };
    const bool ret = http_req(req_url, "", Config::getInstance()._http_timeout_ms, headers, "",
                              resp_code, resp_data);
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
    if (d.HasMember("success") && d["success"].IsBool())
    {
        const bool success = d["success"].GetBool();
        if (success)
            return true;
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return false;
}

bool DnsServiceCloudflare::getZoneId(const std::string & domain_name, std::string & out_zone_id)
{
    const auto & config = Config::getInstance();

    const std::string req_url = fmt::format("{}{}?name={}", API_HOST, API_LIST_ZONES, domain_name);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { fmt::format("Authorization: Bearer {}", _token) };
    const bool ret = http_req(req_url, "", Config::getInstance()._http_timeout_ms, headers, "",
                              resp_code, resp_data);
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
    if (d.HasMember("success") && d["success"].IsBool())
    {
        const bool success = d["success"].GetBool();
        if (success)
        {
            if (d.HasMember("result") && d["result"].IsArray() && !d["result"].GetArray().Empty())
            {
                const auto & result = *d["result"].GetArray().begin();
                if (result.HasMember("id") && result["id"].IsString())
                {
                    out_zone_id = result["id"].GetString();
                    return true;
                }
            }
        }
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return false;
}

bool DnsServiceCloudflare::getRecordId(const std::string & domain_name, const std::string & zone_id,
                                       const std::string & type,
                                       std::string & out_record_id, std::string & out_record_content)
{
    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_LIST_RECORDS, zone_id);
    const std::string req_url = fmt::format("{}{}?type={}&name={}", API_HOST, api_part, type, domain_name);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { fmt::format("Authorization: Bearer {}", _token) };
    const bool ret = http_req(req_url, "", Config::getInstance()._http_timeout_ms, headers, "",
                              resp_code, resp_data);
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
    if (d.HasMember("success") && d["success"].IsBool())
    {
        const bool success = d["success"].GetBool();
        if (success)
        {
            if (d.HasMember("result") && d["result"].IsArray() && !d["result"].GetArray().Empty())
            {
                const auto & result = *d["result"].GetArray().begin();
                if (result.HasMember("id") && result["id"].IsString())
                {
                    out_record_id = result["id"].GetString();
                    if (result.HasMember("content") && result["content"].IsString())
                    {
                        out_record_content = result["content"].GetString();
                        return true;
                    }
                }
            }
        }
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return false;
}

std::string DnsServiceCloudflare::getIp(const std::string & domain, bool is_v4)
{
    const auto sub_domain = get_sub_domain(domain);
    const std::string rec_type = is_v4 ? "A" : "AAAA";
    std::string rec_id_key = domain;
    rec_id_key.append("_");
    rec_id_key.append(rec_type);

    std::string zone_id, record_id, record_content;
    if (_zones.find(sub_domain.first) == _zones.end())
    {
        if (!getZoneId(sub_domain.first, zone_id))
        {
            LOG(WARNING) << "Failed to retrieve zone id of '" << sub_domain.first << "'!";
            return "";
        }
        _zones[sub_domain.first] = zone_id;
    }
    else
        zone_id = _zones[sub_domain.first];

    if (!getRecordId(domain, zone_id, rec_type, record_id, record_content))
    {
        LOG(WARNING) << "Failed to retrieve DNS record id and/or content of '" << rec_id_key << "'!";
        return "";
    }
    _records[rec_id_key] = record_id;

    return record_content;
}

bool DnsServiceCloudflare::setIp(const std::string & domain, const std::string & ip, bool is_v4)
{
    const auto sub_domain = get_sub_domain(domain);
    const std::string rec_type = is_v4 ? "A" : "AAAA";
    std::string rec_id_key = domain;
    rec_id_key.append("_");
    rec_id_key.append(rec_type);

    std::string zone_id, record_id, record_content;
    if (_zones.find(sub_domain.first) == _zones.end())
    {
        LOG(WARNING) << "Missing zone ID of '" << sub_domain.first << "'!";
        return false;
    }
    else
        zone_id = _zones[sub_domain.first];

    if (_records.find(rec_id_key) == _records.end())
    {
        LOG(WARNING) << "Missing DNS record ID of '" << rec_id_key << "'!";
        return false;
    }

    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_PATCH_RECORD, zone_id, record_id);
    const std::string req_url = fmt::format("{}{}", API_HOST, api_part);
    const std::string req_body = fmt::format(R"({{"type":"{}","name":"{}","content":"{}"}})",
                                             rec_type, domain, ip);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { fmt::format("Authorization: Bearer {}", _token) };
    const bool ret = http_req(req_url, req_body, Config::getInstance()._http_timeout_ms, headers, "patch",
                              resp_code, resp_data);
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
    if (d.HasMember("success") && d["success"].IsBool())
    {
        const bool success = d["success"].GetBool();
        if (success)
            return true;
    }

    LOG(WARNING) << "Invalid response '" << resp_data << "'!";
    return false;
}
