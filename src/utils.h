#ifndef PVE_DDNS_CLIENT_SRC_UTILS_H
#define PVE_DDNS_CLIENT_SRC_UTILS_H

#include <string>
#include <vector>

bool str_iequals(const std::string & l, const std::string & r);
bool is_ipv4(const std::string & s);
bool check_hex(const std::string & s);
bool is_ipv6(const std::string & s);
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data);

#endif //PVE_DDNS_CLIENT_SRC_UTILS_H
