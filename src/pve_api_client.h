#ifndef PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
#define PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H

#include <string>

class PveApiClient
{
public:
    PveApiClient() = default;
    ~PveApiClient() = default;

    // Init using infos from global config
    bool init();
    std::pair<std::string, std::string> getHostIp(const std::string & node, const std::string & iface);
    std::pair<std::string, std::string> getGuestIp(const std::string & node,
                                                   int vmid,
                                                   const std::string & iface);
    bool setHostNetworkAddress(const std::string & node, const std::string & iface,
                               const std::string & v4_ip, const std::string & v6_ip);
    bool applyHostNetworkChange(const std::string & node);
    bool revertHostNetworkChange(const std::string & node);

protected:
    bool req(const std::string & api_url, const std::string & req_data, int & resp_code, std::string & resp_data) const;
    bool reqHostNetwork(const std::string & method, const std::string & node) const;
    bool checkApiHost() const;

private:
    // PVE API host (root url)
    std::string _api_host;
    // PVE API access token
    std::string _api_token;
};

#endif //PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
