#ifndef PVE_DDNS_CLIENT_SRC_UTILS_H
#define PVE_DDNS_CLIENT_SRC_UTILS_H

#include <string>
#include <vector>

// Case-insensitive string comparison
bool str_iequals(const std::string & l, const std::string & r);
// Get root and sub-domain from given domain name (www.domain.com => domain.com, www)
std::pair<std::string, std::string> get_sub_domain(const std::string & domain);
// Get DNS service key, current implementation is just colon joined all 3 parameters
std::string get_dns_service_key(const std::string & dns_type,
                                const std::string & api_key, const std::string & api_secret);
// Check if given IP is v4 address
bool is_ipv4(const std::string & s);
// Check if given string is hex string
bool check_hex(const std::string & s);
// Check if given IP is v6 address
bool is_ipv6(const std::string & s);
// HTTP request
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data);
// HTTP request with customizable method, e.g. PUT, DELETE...
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers, const std::string & method,
              int & resp_code, std::string & resp_data);

#endif //PVE_DDNS_CLIENT_SRC_UTILS_H
