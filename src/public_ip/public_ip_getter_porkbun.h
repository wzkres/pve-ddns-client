#ifndef PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_PORKBUN_H
#define PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_PORKBUN_H

#include "public_ip_getter.h"

class PublicIpGetterPorkbun : public IPublicIpGetter
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4() override;
    std::string getIpv6() override;

private:
    std::string _service_name = PUBLIC_IP_GETTER_PORKBUN;
    std::string _api_key;
    std::string _api_secret;
};

#endif //PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_PORKBUN_H
