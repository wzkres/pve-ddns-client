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
    bool checkApiHost() const;

private:
    // PVE API host (root url)
    std::string _api_host;
    // PVE API access token
    std::string _api_token;
};

#endif //PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
