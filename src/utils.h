#ifndef PVE_DDNS_CLIENT_SRC_UTILS_H
#define PVE_DDNS_CLIENT_SRC_UTILS_H

#include <string>
#include <vector>

/// Get app version string
/// \return Version string
std::string get_version_string();

/// Case-insensitive string comparison
/// \param l First string
/// \param r Second string
/// \return Result
bool str_iequals(const std::string & l, const std::string & r);

/// Get root and sub-domain from given domain name (www.domain.com => domain.com, www)
/// \param domain Domain name string
/// \return A pair of strings, first is root, second is sub, may be empty
std::pair<std::string, std::string> get_sub_domain(const std::string & domain);

/// Get DNS service key from type, api key and secret
/// \param dns_type DNS service type
/// \param api_key API key
/// \param api_secret API secret
/// \return Hashed key
size_t get_dns_service_key(const std::string & dns_type, const std::string & api_key, const std::string & api_secret);

/// Check if given IP is v4 address
/// \param s IP address string
/// \return Result
bool is_ipv4(const std::string & s);

/// Check if given string is hex string
/// \param s String to test
/// \return Result
bool check_hex(const std::string & s);

/// Check if given IP is v6 address
/// \param s IP address string
/// \return Result
bool is_ipv6(const std::string & s);

/// HTTP request
/// \param url URL
/// \param req_data Request body
/// \param timeout_ms Timeout
/// \param custom_headers Custom headers
/// \param resp_code Response code
/// \param resp_data Response data
/// \return If request succeeded
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data);

/// HTTP request with customizable method, e.g. PUT, DELETE...
/// \param url URL
/// \param req_data Request body
/// \param timeout_ms Timeout
/// \param custom_headers Custom headers
/// \param method Method name
/// \param resp_code Response code
/// \param resp_data Response data
/// \return If request succeeded
bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers, const std::string & method,
              int & resp_code, std::string & resp_data);

#endif //PVE_DDNS_CLIENT_SRC_UTILS_H
