#include "dns_service_lua.h"

#include <filesystem>

#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "lua.hpp"
#include "../utils.h"
#include "../config.h"
#include "../lua_utils.h"


DnsServiceLua::DnsServiceLua(DnsServiceLua && other) noexcept : _ls(other._ls)
{
    other._ls = nullptr;
}

DnsServiceLua & DnsServiceLua::operator=(DnsServiceLua && other) noexcept
{
    if (this != &other) 
    {
        lua_close(_ls);
        _ls = other._ls;
        other._ls = nullptr;
    }
    return *this;
}

DnsServiceLua::~DnsServiceLua()
{
    SPDLOG_INFO("dtor");
    lua_uninit_module(_ls);
}

bool DnsServiceLua::loadModule(const std::string & module_name)
{
    std::filesystem::path mdl_path = Config::getInstance()._module_path_dns;
    std::string file_name = module_name;
    file_name.append(".lua");
    mdl_path /= file_name;

    _ls = lua_load_module("LUA DNS service", mdl_path.string());
    if (nullptr == _ls)
    {
        SPDLOG_WARN("Failed to lua_load_module {}!", mdl_path.string());
        return false;
    }
    
    return true;
}

const std::string &DnsServiceLua::getServiceName()
{
    return _service_name;
}

bool DnsServiceLua::setCredentials(const std::string & cred_str)
{
    return lua_moudule_set_credentials(_ls, cred_str);
}

std::string DnsServiceLua::getIpv4(const std::string & domain)
{
    std::string out_ip;
    if (!getIp("get_ipv4", domain, out_ip))
    {
        SPDLOG_WARN("Failed to get_ipv4 for domain {}!", domain);
        return "";
    }

    if (!is_ipv4(out_ip))
    {
        SPDLOG_WARN("'{}' is not valid IPv4 ip!", out_ip);
        return "";
    }

    return out_ip;
}

std::string DnsServiceLua::getIpv6(const std::string & domain)
{
    std::string out_ip;
    if (!getIp("get_ipv6", domain, out_ip))
    {
        SPDLOG_WARN("Failed to get_ipv6 for domain {}!", domain);
        return "";
    }

    if (!is_ipv4(out_ip))
    {
        SPDLOG_WARN("'{}' is not valid IPv6 ip!", out_ip);
        return "";
    }

    return out_ip;
}

bool DnsServiceLua::setIpv4(const std::string & domain, const std::string & ip)
{
    return setIp("set_ipv4", domain, ip);
}

bool DnsServiceLua::setIpv6(const std::string & domain, const std::string & ip)
{
    return setIp("set_ipv6", domain, ip);
}

bool DnsServiceLua::getIp(const std::string & type, const std::string & domain, std::string & out_ip)
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

    lua_pushstring(_ls, domain.c_str());
    if (lua_pcall(_ls, 1, 1, 0) != LUA_OK)
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

bool DnsServiceLua::setIp(const std::string & type, const std::string & domain, const std::string & ip)
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

    lua_pushstring(_ls, domain.c_str());
    lua_pushstring(_ls, ip.c_str());
    if (lua_pcall(_ls, 2, 1, 0) != LUA_OK)
    {
        SPDLOG_WARN("Error calling function '{}': {}!", type, lua_tostring(_ls, -1));
        return false;
    }

    if (!lua_isboolean(_ls, -1))
    {
        SPDLOG_WARN("'{}' did not return a boolean!", type);
        lua_pop(_ls, 1);
        return false;
    }
    
    bool result = lua_toboolean(_ls, -1);
    lua_pop(_ls, 1);
    return result;
}
