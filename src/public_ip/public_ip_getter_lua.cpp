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

    _ls = lua_init_module(mdl_path.string());
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Failed to lua_init_module {}", mdl_path.string());
        return false;
    }

    auto mi = lua_module_info(_ls);
    if (mi.first.empty() || mi.second.empty())
    {
        SPDLOG_WARN("Invalid module info from LUA public IP getter module {}!", module_name);
        return false;
    }

    SPDLOG_INFO("LUA public IP getter module '{}' loaded, author: {}, description: {}.", 
        module_name, mi.first, mi.second);
    return true;
}

const std::string & PublicIpGetterLua::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterLua::setCredentials(const std::string & cred_str)
{
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Invalid _ls!");
        return false;
    }
    if (cred_str.empty())
    {
        SPDLOG_WARN("Credentials string is empty!");
        return false;
    }

    constexpr const char * func_name = "set_credentials";
    if (lua_getglobal(_ls, func_name) != LUA_TFUNCTION)
    {
        SPDLOG_WARN("Missing function '{}' in LUA module!", func_name);
        return false;
    }

    lua_pushstring(_ls, cred_str.c_str());
    if (lua_pcall(_ls, 1, 1, 0) != LUA_OK)
    {
        SPDLOG_WARN("Error calling function '{}': {}!", func_name, lua_tostring(_ls, -1));
        return false;
    }

    if (!lua_isboolean(_ls, -1))
    {
        SPDLOG_WARN("'{}' did not return a boolean!", func_name);
        lua_pop(_ls, 1);
        return false;
    }
    
    bool ret = lua_toboolean(_ls, -1);
    lua_pop(_ls, 1);

    return ret;
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
