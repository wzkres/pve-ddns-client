#include "public_ip_getter_lua.h"

#include <filesystem>

#include "spdlog/spdlog.h"
#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "lua.hpp"
#include "../utils.h"
#include "../config.h"
#include "../lua_utils.h"

PublicIpGetterLua::PublicIpGetterLua(PublicIpGetterLua &&other) noexcept
{
    other._ls = nullptr;
}

PublicIpGetterLua &PublicIpGetterLua::operator=(PublicIpGetterLua &&other) noexcept
{
    if (this != &other) 
    {
        lua_close(_ls);
        _ls = other._ls;
        other._ls = nullptr;
    }
    return *this;
}

PublicIpGetterLua::~PublicIpGetterLua()
{
    SPDLOG_INFO("dtor");
    lua_uninit_module(_ls);
}

bool PublicIpGetterLua::loadModule(const std::string & module_name)
{
    std::filesystem::path mdl_path = Config::getInstance()._module_path_ip;
    std::string file_name = module_name;
    file_name.append(".lua");
    mdl_path /= file_name;

    _ls = lua_load_module("LUA public IP getter", mdl_path.string());
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Failed to lua_load_module {}!", mdl_path.string());
        return false;
    }

    return true;
}

const std::string & PublicIpGetterLua::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterLua::setCredentials(const std::string & cred_str)
{
    return lua_moudule_set_credentials(_ls, cred_str);
}

std::string PublicIpGetterLua::getIpv4()
{
    std::string out_ip;

    if (!getIp("get_ipv4", out_ip))
    {
        SPDLOG_WARN("Failed to get_ipv4!");
        return "";
    }

    if (!is_ipv4(out_ip))
    {
        SPDLOG_WARN("'{}' is not valid IPv4 ip!", out_ip);
        return "";
    }

    return out_ip;
}

std::string PublicIpGetterLua::getIpv6()
{
    std::string out_ip;
    
    if (!getIp("get_ipv6", out_ip))
    {
        SPDLOG_WARN("Failed to get_ipv6!");
        return "";
    }

    if (!is_ipv6(out_ip))
    {
        SPDLOG_WARN("'{}' is not valid IPv6 ip!", out_ip);
        return "";
    }

    return out_ip;
}

bool PublicIpGetterLua::getIp(const std::string & type, std::string & out_ip)
{
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Invalid _ls!");
        return false;
    }

    if (lua_getglobal(_ls, type.c_str()) != LUA_TFUNCTION)
    {
        SPDLOG_WARN("Missing function '{}' in LUA module!", type);
        return false;
    }

    if (lua_pcall(_ls, 0, 1, 0) != LUA_OK)
    {
        SPDLOG_WARN("Error calling function '{}': {}!", type, lua_tostring(_ls, -1));
        return false;
    }

    if (!lua_isstring(_ls, -1))
    {
        SPDLOG_WARN("'{}' did not return a string!", type);
        lua_pop(_ls, 1);
        return false;
    }
    
    out_ip = lua_tostring(_ls, -1);
    lua_pop(_ls, 1);

    return true;
}
