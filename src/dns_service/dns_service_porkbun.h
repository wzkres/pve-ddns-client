#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H

#include "dns_service.h"

class DnsServicePorkbun : public IDnsService
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4(const std::string & domain) override;
    std::string getIpv6(const std::string & domain) override;
    bool setIpv4(const std::string & domain) override;
    bool setIpv6(const std::string & domain) override;

protected:

private:
    std::string _service_name = DNS_SERVICE_PORKBUN;
    std::string _api_key;
    std::string _api_secret;
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H
