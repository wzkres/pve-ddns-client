#include "public_ip_getter.h"

#include "spdlog/spdlog.h"

#include "../utils.h"
#include "../config.h"
#include "../lua_utils.h"
#include "public_ip_getter_iface.h"
#include "public_ip_getter_porkbun.h"
#include "public_ip_getter_ipify.h"
#include "public_ip_getter_lua.h"

IPublicIpGetter * PublicIpGetterFactory::create(const std::string & service_name)
{
    if (service_name.empty())
    {
        SPDLOG_WARN("Invalid service_name!");
        return nullptr;
    }

    if (str_iequals(service_name, PUBLIC_IP_GETTER_IFACE))
    {
        auto * getter = new(std::nothrow) PublicIpGetterIface();
        if (nullptr == getter)
        {
            SPDLOG_ERROR("Failed to instantiate PublicIpGetterIface!");
            return nullptr;
        }
        return getter;
    }

    if (str_iequals(service_name, PUBLIC_IP_GETTER_PORKBUN))
    {
        auto * getter = new(std::nothrow) PublicIpGetterPorkbun();
        if (nullptr == getter)
        {
            SPDLOG_ERROR("Failed to instantiate PublicIpGetterPorkbun!");
            return nullptr;
        }
        return getter;
    }

    if (str_iequals(service_name, PUBLIC_IP_GETTER_IPIFY))
    {
        auto * getter = new(std::nothrow) PublicIpGetterIpify();
        if (nullptr == getter)
        {
            SPDLOG_ERROR("Failed to instantiate PublicIpGetterIpify!");
            return nullptr;
        }
        return getter;
    }

    // Try loading the LUA module with the service_name
    auto * getter = new(std::nothrow) PublicIpGetterLua();
    if (nullptr == getter)
    {
        SPDLOG_ERROR("Failed to instantiate PublicIpGetterLua!");
        return nullptr;
    }
    if (!getter->loadModule(service_name))
    {
        SPDLOG_WARN("Failed to load public IP getter LUA module {}!", service_name);
        delete getter;
    }
    else
    {
        return getter;
    }

    SPDLOG_WARN("Unsupported public ip getter '{}'!", service_name);

    return nullptr;
}

void PublicIpGetterFactory::destroy(IPublicIpGetter * ip_getter)
{
    if (nullptr == ip_getter)
    {
        SPDLOG_WARN("Invalid param!");
        return;
    }

    const std::string & name = ip_getter->getServiceName();
    if (str_iequals(name, PUBLIC_IP_GETTER_IFACE))
    {
        auto * g = dynamic_cast<PublicIpGetterIface *>(ip_getter);
        if (nullptr == g)
            SPDLOG_WARN("ip_getter is not instance of PublicIpGetterIface!");
        delete g;
    }
    else if (str_iequals(name, PUBLIC_IP_GETTER_PORKBUN))
    {
        auto * g = dynamic_cast<PublicIpGetterPorkbun *>(ip_getter);
        if (nullptr == g)
            SPDLOG_WARN("ip_getter is not instance of PublicIpGetterPorkbun!");
        delete g;
    }
    else if (str_iequals(name, PUBLIC_IP_GETTER_IPIFY))
    {
        auto * g = dynamic_cast<PublicIpGetterIpify *>(ip_getter);
        if (nullptr == g)
            SPDLOG_WARN("ip_getter is not instance of PublicIpGetterIpify!");
        delete g;
    }
    else if (str_iequals(name, PUBLIC_IP_GETTER_LUA))
    {
        auto * g = dynamic_cast<PublicIpGetterLua *>(ip_getter);
        if (nullptr == g)
            SPDLOG_WARN("ip_getter is not instance of PublicIpGetterLua!");
        delete g;
    }
    else
        SPDLOG_WARN("Unsupported public ip getter '{}'!", name);
}
