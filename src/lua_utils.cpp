#include "lua_utils.h"

#include "spdlog/spdlog.h"
#include "lua.hpp"

#include "config.h"
#include "utils.h"

extern "C" int luaopen_rapidjson(lua_State * L);

static int spdlog_lua(lua_State * ls)
{
    const char * src_file = luaL_checkstring(ls, 1);
    const std::uint_least32_t src_line = static_cast<std::uint_least32_t>(luaL_checkinteger(ls, 2));
    const char * src_func = luaL_checkstring(ls, 3);
    const int log_lvl = static_cast<int>(luaL_checkinteger(ls, 4));
    const char * log_str = luaL_checkstring(ls, 5);

    spdlog::global_logger()->log(SPDLOG_NAMESPACE::source_loc{src_file, src_line, src_func}, 
        static_cast<spdlog::level>(log_lvl), "{}", log_str);

    return 0;
}

bool lua_open_log_api(lua_State * ls)
{
    if (nullptr == ls)
    {
        SPDLOG_WARN("Invalid param ls!");
        return false;
    }
    
    lua_pushinteger(ls, SPDLOG_LEVEL_TRACE);
    lua_setglobal(ls, "LOG_TRACE");

    lua_pushinteger(ls, SPDLOG_LEVEL_DEBUG);
    lua_setglobal(ls, "LOG_DEBUG");

    lua_pushinteger(ls, SPDLOG_LEVEL_INFO);
    lua_setglobal(ls, "LOG_INFO");

    lua_pushinteger(ls, SPDLOG_LEVEL_WARN);
    lua_setglobal(ls, "LOG_WARN");

    lua_pushinteger(ls, SPDLOG_LEVEL_ERROR);
    lua_setglobal(ls, "LOG_ERROR");

    lua_pushinteger(ls, SPDLOG_LEVEL_CRITICAL);
    lua_setglobal(ls, "LOG_CRITICAL");

    lua_pushcfunction(ls, spdlog_lua);
    lua_setglobal(ls, "spdlog");

    const char * lua_log_src = R"|(
function LOG_WITH_LEVEL(level, message)
    -- Level 3 gets the info of the function that called LOGD, LOGI, LOGW...
    local info = debug.getinfo(3, "Sln")
    
    -- Format the file source (remove leading '@' if present)
    local file = info.source:gsub("^@", "")
    local line = info.currentline
    local func = info.name or "(anonymous/main)"

    spdlog(file, line, func, level, message)
end

function LOGD(message)
    LOG_WITH_LEVEL(LOG_DEBUG, message)
end

function LOGI(message)
    LOG_WITH_LEVEL(LOG_INFO, message)
end

function LOGW(message)
    LOG_WITH_LEVEL(LOG_WARN, message)
end

function LOGE(message)
    LOG_WITH_LEVEL(LOG_ERROR, message)
end)|";

    if (luaL_dostring(ls, lua_log_src) != LUA_OK)
    {
        SPDLOG_WARN("LUA error: {}", lua_tostring(ls, -1));
        lua_pop(ls, 1);
    }

    return true;
}

static int http_request(lua_State * ls)
{
    std::string req_url, req_body;
    std::vector<std::string> req_headers;

    req_url = luaL_checkstring(ls, 1);
    // Optional body
    if (lua_type(ls, 2) == LUA_TSTRING)
    {
        req_body = lua_tostring(ls, 2);
    }
    // Optional headers
    if (lua_type(ls, 3) == LUA_TTABLE)
    {
        size_t len = lua_rawlen(ls, 3);
        for (size_t i = 1; i <= len; ++i)
        {
            if (lua_rawgeti(ls, 3, i) == LUA_TSTRING)
                req_headers.push_back(lua_tostring(ls, -1));
            lua_pop(ls, 1);
        }
    }

    int resp_code = 0;
    std::string resp_data;
    bool ret = http_req(req_url, req_body, Config::getInstance()._http_timeout_ms, req_headers, resp_code, resp_data);

    lua_pushboolean(ls, ret);
    lua_pushinteger(ls, resp_code);
    lua_pushstring(ls, resp_data.c_str());

    return 3;
}

bool lua_open_http_api(lua_State * ls)
{
    if (nullptr == ls)
    {
        SPDLOG_WARN("Invalid param ls!");
        return false;
    }

    lua_pushcfunction(ls, http_request);
    lua_setglobal(ls, "http_request");

    return true;
}

lua_State * lua_init_module(const std::string & module_path)
{
    auto * ls = luaL_newstate();
    if (nullptr == ls)
    {
        SPDLOG_ERROR("Failed to luaL_newstate for module {}!", module_path);
        return nullptr;
    }
    luaL_openlibs(ls);

    if (!lua_open_log_api(ls))
    {
        lua_close(ls);
        SPDLOG_WARN("Failed to lua_open_log_api for module {}!", module_path);
        return false;
    }

    if (!lua_open_http_api(ls))
    {
        lua_close(ls);
        SPDLOG_WARN("Failed to lua_open_http_api for module {}!", module_path);
        return false;
    }

    // Manually open lua-rapidjson as it is compiled with main executable
    luaL_requiref(ls, "rapidjson", luaopen_rapidjson, 1);

    // Load the module lua source
    if (luaL_dofile(ls, module_path.c_str()) != LUA_OK) 
    {
        SPDLOG_WARN("LUA error: {} when luaL_dofile {}", lua_tostring(ls, -1), module_path);
        lua_pop(ls, 1);
        lua_close(ls);
        return nullptr;
    }

    return ls;
}

void lua_uninit_module(lua_State * ls)
{
    if (nullptr != ls)
        lua_close(ls);
}

std::pair<std::string, std::string> lua_module_info(lua_State * ls)
{
    if (nullptr == ls)
    {
        SPDLOG_WARN("Invalid param ls!");
        return {};
    }

    constexpr const char * func_name = "module_info";
    if (lua_getglobal(ls, func_name) != LUA_TFUNCTION)
    {
        SPDLOG_WARN("Missing function '{}' in LUA module!", func_name);
        return {};
    }

    if (lua_pcall(ls, 0, 1, 0) != LUA_OK)
    {
        SPDLOG_WARN("Error calling function '{}': {}!", func_name, lua_tostring(ls, -1));
        return {};
    }

    if (!lua_istable(ls, -1))
    {
        SPDLOG_WARN("'{}' did not return a table!", func_name);
        lua_pop(ls, 1);
        return {};
    }

    if (lua_rawlen(ls, -1) != 2)
    {
        SPDLOG_WARN("'{}' did not return a array with 2 elements!", func_name);
        lua_pop(ls, 1);
        return {};
    }

    std::string author, desc;
    if (lua_rawgeti(ls, -1, 1) != LUA_TSTRING)
    {
        SPDLOG_WARN("'{}' invalid return, first element is not string!", func_name);
        lua_pop(ls, 1);
        return {};
    }
    author = lua_tostring(ls, -1);
    lua_pop(ls, 1);

    if (lua_rawgeti(ls, -1, 2) != LUA_TSTRING)
    {
        SPDLOG_WARN("'{}' invalid return, second element is not string!", func_name);
        lua_pop(ls, 1);
        return {};
    }
    desc = lua_tostring(ls, -1);
    lua_pop(ls, 2);

    return {author, desc};
}
