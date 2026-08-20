#ifndef PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_LUA_H
#define PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_LUA_H

#include "notify_service.h"


typedef struct lua_State lua_State;

/// Notify service using a LUA module
class NotifyServiceLua : public INotifyService
{
public:
    NotifyServiceLua() = default;
    NotifyServiceLua(const NotifyServiceLua & other) = delete;
    NotifyServiceLua & operator=(const NotifyServiceLua & other) = delete;
    NotifyServiceLua(NotifyServiceLua && other) noexcept;
    NotifyServiceLua & operator=(NotifyServiceLua && other) noexcept;
    virtual ~NotifyServiceLua();

    bool loadModule(const std::string & module_name);

    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    bool notifyIpChange(bool is_v6, const std::string &domain,
                        const std::string & old_ip, const std::string & new_ip) override;

private:
    /// Service name
    std::string _service_name = NOTIFY_SERVICE_LUA;
    lua_State * _ls;
};

#endif //PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_LUA_H
