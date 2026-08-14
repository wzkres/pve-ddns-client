#ifndef PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_LUA_H
#define PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_LUA_H

#include "public_ip_getter.h"

typedef struct lua_State lua_State;

/// Public IP getter using a LUA module
class PublicIpGetterLua : public IPublicIpGetter
{
public:
    virtual ~PublicIpGetterLua();

    bool loadModule(const std::string & module_name);

    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4() override;
    std::string getIpv6() override;

protected:
    bool getIp(const std::string & type, std::string & out_ip);

private:
    /// Service name
    std::string _service_name = PUBLIC_IP_GETTER_LUA;
    lua_State * _ls;
};

#endif //PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_LUA_H
