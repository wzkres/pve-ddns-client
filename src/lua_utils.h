#ifndef PVE_DDNS_CLIENT_SRC_LUA_UTILS_H
#define PVE_DDNS_CLIENT_SRC_LUA_UTILS_H

#include <string>
// #include <utility>

typedef struct lua_State lua_State;

/// \brief spdlog API LUA binding
/// \param ls lua_State*
/// \return Result
bool lua_open_log_api(lua_State * ls);

/// \brief HTTP request API LUA binding
/// \param ls lua_State*
/// \return Result
bool lua_open_http_api(lua_State * ls);

/// \brief Load a LUA service module
/// \param module_path Full path to .lua file
/// \return On success return lua_State*, otherwise return nullptr
lua_State * lua_init_module(const std::string & module_path);

/// \brief Unload LUA service module instance
/// \param ls lua_State* of the loaded module
void lua_uninit_module(lua_State * ls);

/// \brief Get module author and description
/// \param ls lua_State* of the loaded module
/// \return A pair of std::string, first is author, second is description
std::pair<std::string, std::string> lua_module_info(lua_State * ls);

#endif //PVE_DDNS_CLIENT_SRC_LUA_UTILS_H
