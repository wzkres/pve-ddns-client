#ifndef PVE_DDNS_CLIENT_SRC_LUA_UTILS_H
#define PVE_DDNS_CLIENT_SRC_LUA_UTILS_H

#include <string>
// #include <utility>

typedef struct lua_State lua_State;

/// \brief spdlog API LUA binding
/// \param ls lua_State*
/// \return Boolean result
bool lua_open_log_api(lua_State * ls);

/// \brief HTTP request API LUA binding
/// \param ls lua_State*
/// \return Boolean result
bool lua_open_http_api(lua_State * ls);

/// \brief Load a LUA service module
/// \param module_path Full path to .lua file
/// \return On success return lua_State*, otherwise return nullptr
lua_State * lua_init_module(const std::string & module_path);

/// \brief Unload LUA service module instance
/// \param ls lua_State* of the loaded module
void lua_uninit_module(lua_State * ls);

/// \brief Load LUA module helper function
/// \param type Type string used for logging
/// \param module_path Full path to .lua file
/// \return On success return lua_State*, otherwise return nullptr
lua_State * lua_load_module(const std::string & type, const std::string & module_path);

/// \brief Set credentials for a loaded LUA module
/// \param ls lua_State*
/// \param cred_str Credentials string
/// \return Boolean result
bool lua_moudule_set_credentials(lua_State * ls, const std::string & cred_str);

/// \brief Get module author and description
/// \param ls lua_State* of the loaded module
/// \return A pair of std::string, first is author, second is description
std::pair<std::string, std::string> lua_module_info(lua_State * ls);

#endif //PVE_DDNS_CLIENT_SRC_LUA_UTILS_H
