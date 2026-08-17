#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_LUA_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_LUA_H

#include "dns_service.h"

typedef struct lua_State lua_State;

/// LUA Module DNS service implementation
class DnsServiceLua : public IDnsService
{
public:
    virtual ~DnsServiceLua();

    bool loadModule(const std::string & module_name);

    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4(const std::string & domain) override;
    std::string getIpv6(const std::string & domain) override;
    bool setIpv4(const std::string & domain, const std::string & ip) override;
    bool setIpv6(const std::string & domain, const std::string & ip) override;

protected:
    bool getIp(const std::string & type, const std::string & domain, std::string & out_ip);
    bool setIp(const std::string & type, const std::string & domain, const std::string & ip);

private:
    /// Service name
    std::string _service_name = DNS_SERVICE_LUA;
    lua_State * _ls;
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_LUA_H
