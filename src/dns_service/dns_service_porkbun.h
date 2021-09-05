#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H

#include "dns_service.h"

/// Porkbun DNS service implementation
class DnsServicePorkbun : public IDnsService
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4(const std::string & domain) override;
    std::string getIpv6(const std::string & domain) override;
    bool setIpv4(const std::string & domain, const std::string & ip) override;
    bool setIpv6(const std::string & domain, const std::string & ip) override;

protected:
    std::string getIp(const std::string & domain, bool is_v4);
    bool setIp(const std::string & domain, const std::string & ip, bool is_v4);

private:
    /// Service name
    std::string _service_name = DNS_SERVICE_PORKBUN;
    /// Porkbun API key
    std::string _api_key;
    /// Porkbun secret key
    std::string _api_secret;
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_PORKBUN_H
