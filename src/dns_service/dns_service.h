#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H

#include <string>

constexpr const char * DNS_SERVICE_PORKBUN = "porkbun";

class IDnsService
{
public:
    virtual const std::string & getServiceName() = 0;
    virtual bool setCredentials(const std::string & cred_str) = 0;
};

class DnsServiceFactory
{
public:
    static IDnsService * create(const std::string & service_name);
    static void destroy(IDnsService * dns_service);
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H
