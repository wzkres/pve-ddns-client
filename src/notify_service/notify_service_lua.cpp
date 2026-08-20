#include "notify_service_lua.h"

#include <filesystem>

#include "spdlog/spdlog.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "lua.hpp"
#include "../utils.h"
#include "../config.h"
#include "../lua_utils.h"


NotifyServiceLua::NotifyServiceLua(NotifyServiceLua && other) noexcept
{
    other._ls = nullptr;
}

NotifyServiceLua & NotifyServiceLua::operator=(NotifyServiceLua && other) noexcept
{
    if (this != &other) 
    {
        lua_close(_ls);
        _ls = other._ls;
        other._ls = nullptr;
    }
    return *this;
}

NotifyServiceLua::~NotifyServiceLua()
{
    SPDLOG_INFO("dtor");
    lua_uninit_module(_ls);
}

bool NotifyServiceLua::loadModule(const std::string & module_name)
{
    std::filesystem::path mdl_path = Config::getInstance()._module_path_notify;
    std::string file_name = module_name;
    file_name.append(".lua");
    mdl_path /= file_name;

    _ls = lua_load_module("LUA notify service", mdl_path.string());
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Failed to lua_load_module {}!", mdl_path.string());
        return false;
    }

    return true;
}

const std::string & NotifyServiceLua::getServiceName()
{
    return _service_name;
}

bool NotifyServiceLua::setCredentials(const std::string & cred_str)
{
    return lua_moudule_set_credentials(_ls, cred_str);
}

bool NotifyServiceLua::notifyIpChange(bool is_v6, const std::string & domain,
                                      const std::string & old_ip, const std::string & new_ip)
{
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Invalid _ls!");
        return false;
    }

    if (lua_getglobal(_ls, "notify_ip_change") != LUA_TFUNCTION)
    {
        SPDLOG_WARN("Missing function notify_ip_change in LUA module!");
        return false;
    }

    lua_pushboolean(_ls, is_v6);
    lua_pushstring(_ls, domain.c_str());
    lua_pushstring(_ls, old_ip.c_str());
    lua_pushstring(_ls, new_ip.c_str());
    if (lua_pcall(_ls, 4, 1, 0) != LUA_OK)
    {
        SPDLOG_WARN("Error calling function 'notify_ip_change': {}!", lua_tostring(_ls, -1));
        return false;
    }

    if (!lua_isboolean(_ls, -1))
    {
        SPDLOG_WARN("'notify_ip_change' did not return a boolean!");
        lua_pop(_ls, 1);
        return false;
    }
    
    bool result = lua_toboolean(_ls, -1);
    lua_pop(_ls, 1);

    return result;
}
