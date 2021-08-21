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

protected:
    bool req(const std::string & api_url, const std::string & req_data, int & resp_code, std::string & resp_data) const;

private:
    // PVE API host (root url)
    std::string _api_host;
    // PVE API access token
    std::string _api_token;
    // Default http request timeout 30s
    long _req_timeout = 30000;
};

#endif //PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
