#ifndef PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_IPIFY_H
#define PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_IPIFY_H

#include "public_ip_getter.h"

/// Public IP getter using ipify APIs
class PublicIpGetterIpify : public IPublicIpGetter
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4() override;
    std::string getIpv6() override;

protected:
    static std::string getIp(const std::string & api_host);

private:
    /// Service name
    std::string _service_name = PUBLIC_IP_GETTER_IPIFY;
};

#endif //PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_IPIFY_H
