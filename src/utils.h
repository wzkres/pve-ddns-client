#ifndef PVE_DDNS_CLIENT_SRC_UTILS_H
#define PVE_DDNS_CLIENT_SRC_UTILS_H

#include <string>
#include <vector>

bool str_iequals(const std::string & l, const std::string & r);
std::pair<std::string, std::string> get_sub_domain(const std::string & domain);
std::string get_dns_service_key(const std::string & dns_type,
                                const std::string & api_key, const std::string & api_secret);
bool is_ipv4(const std::string & s);
bool check_hex(const std::string & s);
bool is_ipv6(const std::string & s);
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data);
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers, const std::string & method,
              int & resp_code, std::string & resp_data);

#endif //PVE_DDNS_CLIENT_SRC_UTILS_H
