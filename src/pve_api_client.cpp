#include "pve_api_client.h"

#include "glog/logging.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "config.h"
#include "utils.h"

static const char * API_VERSION = "api2/json/version";
static const char * API_HOST_NETWORK = "api2/json/nodes/{}/network/{}";
static const char * API_HOST_NETWORK_APPLY = "api2/json/nodes/{}/network";
static const char * API_GUEST_NETWORK = "api2/json//nodes/{}/qemu/{}/agent/network-get-interfaces";

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

std::pair<std::string, std::string> PveApiClient::getHostIp(const std::string & node, const std::string & iface)
{
    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_HOST_NETWORK, node, iface);
    const std::string req_url = fmt::format("{}{}", config._pve_api_host, api_part);

    int resp_code = 0;
    std::string resp_data;
    const bool ret = req(req_url, "", resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << ", response is "
                     << resp_data << "!";
        return { "", "" };
    }

    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        LOG(WARNING) << "Failed to parse response json, error '" << rapidjson::GetParseError_En(ok.Code())
                     << "' (" << ok.Offset() << ")";
        return { "", "" };
    }

    if (d.HasMember("data") && d["data"].IsObject())
    {
        const auto & data = d["data"];
        std::string v4_ip, v6_ip;
        if (data.HasMember("address") && data["address"].IsString())
            v4_ip = data["address"].GetString();
        if (data.HasMember("address6") && data["address6"].IsString())
            v6_ip = data["address6"].GetString();

        return { v4_ip, v6_ip };
    }

    return { "", "" };
}

std::pair<std::string, std::string> PveApiClient::getGuestIp(const std::string & node,
                                                             int vmid,
                                                             const std::string & iface)
{
    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_GUEST_NETWORK, node, vmid);
    const std::string req_url = fmt::format("{}{}", config._pve_api_host, api_part);

    int resp_code = 0;
    std::string resp_data;
    const bool ret = req(req_url, "", resp_code, resp_data);
    if (!ret || 200 != resp_code)
    {
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << ", response is "
                     << resp_data << "!";
        return { "", "" };
    }

    rapidjson::Document d;
    rapidjson::ParseResult ok = d.Parse(resp_data.c_str());
    if (!ok)
    {
        LOG(WARNING) << "Failed to parse response json, error '" << rapidjson::GetParseError_En(ok.Code())
                     << "' (" << ok.Offset() << ")";
        return { "", "" };
    }

    if (d.HasMember("data") && d["data"].IsObject())
    {
        const auto & data = d["data"];
        std::string v4_ip, v6_ip;
        if (data.HasMember("result") && data["result"].IsArray())
        {
            const auto & result = data["result"].GetArray();
            for (const auto & r : result)
            {
                if (r.HasMember("name") && r["name"].IsString() && r["name"].GetString() == iface)
                {
                    if (r.HasMember("ip-addresses") && r["ip-addresses"].IsArray())
                    {
                        const auto & ips = r["ip-addresses"].GetArray();
                        for (const auto & ip : ips)
                        {
                            if (ip.HasMember("ip-address-type") && ip["ip-address-type"].IsString())
                            {
                                const std::string type = ip["ip-address-type"].GetString();
                                if ("ipv4" == type)
                                    v4_ip = ip["ip-address"].GetString();
                                else if ("ipv6" == type && v6_ip.empty())
                                {
                                    v6_ip = ip["ip-address"].GetString();
                                    if (v6_ip.compare(0, 4, "fe80") == 0)
                                        v6_ip.clear();
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        return { v4_ip, v6_ip };
    }

    return { "", "" };
}

bool PveApiClient::setHostNetworkAddress(const std::string & node, const std::string & iface,
                                         const std::string & v4_ip, const std::string & v6_ip)
{
    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_HOST_NETWORK, node, iface);
    const std::string req_url = fmt::format("{}{}", config._pve_api_host, api_part);
    const std::string req_body = fmt::format(R"(type=bridge&address6={}&netmask6=128&address={}&netmask=255.255.255.0)",
                                             v6_ip, v4_ip);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { get_pve_api_http_auth_header() };
    const bool ret = http_req(req_url, req_body, Config::getInstance()._http_timeout_ms, headers, "put",
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

//    if (d.HasMember("data") && d["data"].IsObject())
//    {
//    }

    return true;
}

bool PveApiClient::applyHostNetworkChange(const std::string & node)
{
    return reqHostNetwork("put", node);
}

bool PveApiClient::revertHostNetworkChange(const std::string & node)
{
    return reqHostNetwork("delete", node);
}

bool PveApiClient::req(const std::string & api_url, const std::string & req_data,
                       int & resp_code, std::string & resp_data) const
{
    std::vector<std::string> headers = { get_pve_api_http_auth_header() };
    return http_req(api_url, req_data, Config::getInstance()._http_timeout_ms, headers, resp_code, resp_data);
}

bool PveApiClient::reqHostNetwork(const std::string & method, const std::string & node) const
{
    const auto & config = Config::getInstance();

    const std::string api_part = fmt::format(API_HOST_NETWORK_APPLY, node);
    const std::string req_url = fmt::format("{}{}", config._pve_api_host, api_part);

    int resp_code = 0;
    std::string resp_data;
    std::vector<std::string> headers = { get_pve_api_http_auth_header() };
    const bool ret = http_req(req_url, "", Config::getInstance()._http_timeout_ms, headers, method,
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

//    if (d.HasMember("data") && d["data"].IsObject())
//    {
//    }

    return true;
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
        LOG(WARNING) << "Failed to request '" << req_url << "', response code is " << resp_code << ", response is "
                     << resp_data << "!";
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
