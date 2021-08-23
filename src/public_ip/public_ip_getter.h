#ifndef PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H
#define PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H

#include <string>

constexpr const char * PUBLIC_IP_GETTER_PORKBUN = "porkbun";

class IPublicIpGetter
{
public:
    virtual const std::string & getServiceName() = 0;
    virtual bool setCredentials(const std::string & cred_str) = 0;
    virtual std::string getIpv4() = 0;
    virtual std::string getIpv6() = 0;
};

class PublicIpGetterFactory
{
public:
    static IPublicIpGetter * create(const std::string & service_name);
    static void destroy(IPublicIpGetter * ip_getter);
};

#endif //PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H
