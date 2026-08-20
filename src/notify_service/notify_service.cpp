#include "notify_service.h"

#include "spdlog/spdlog.h"

#include "../utils.h"
#include "../config.h"
#include "../lua_utils.h"
#include "notify_service_lua.h"


INotifyService * NotifyServiceFactory::create(const std::string & service_name)
{
    if (service_name.empty())
    {
        SPDLOG_WARN("Invalid service_name!");
        return nullptr;
    }

    // Try loading the LUA module with the service_name
    auto * notify = new(std::nothrow) NotifyServiceLua();
    if (nullptr == notify)
    {
        SPDLOG_ERROR("Failed to instantiate NotifyServiceLua!");
        return nullptr;
    }
    if (!notify->loadModule(service_name))
    {
        SPDLOG_WARN("Failed to load notify service LUA module {}!", service_name);
        delete notify;
    }
    else
    {
        return notify;
    }

    SPDLOG_WARN("Unsupported notify service '{}'!", service_name);

    return nullptr;
}

void NotifyServiceFactory::destroy(INotifyService * notify_service)
{
    if (nullptr == notify_service)
    {
        SPDLOG_WARN("Invalid param!");
        return;
    }

    const std::string & name = notify_service->getServiceName();
    if (str_iequals(name, NOTIFY_SERVICE_LUA))
    {
        auto * g = dynamic_cast<NotifyServiceLua *>(notify_service);
        if (nullptr == g)
            SPDLOG_WARN("notify_service is not instance of NotifyServiceLua!");
        delete g;
    }
    else
        SPDLOG_WARN("Unsupported notify service '{}'!", name);
}
