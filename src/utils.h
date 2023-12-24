#ifndef PVE_DDNS_CLIENT_SRC_UTILS_H
#define PVE_DDNS_CLIENT_SRC_UTILS_H

#include <string>
#include <vector>

/// \brief Get app version string
/// \return Version string
std::string get_version_string();

/// \brief Case-insensitive string comparison
/// \param l First string
/// \param r Second string
/// \return Result
bool str_iequals(const std::string & l, const std::string & r);

/// \brief Get root and sub-domain from given domain name (www.domain.com => domain.com, www)
/// \param domain Domain name string
/// \return A pair of strings, first is root, second is sub, may be empty
std::pair<std::string, std::string> get_sub_domain(const std::string & domain);

/// \brief Get DNS service key from type, api key and secret
/// \param dns_type DNS service type
/// \param credentials DNS service credentials
/// \return Hashed key
size_t get_dns_service_key(const std::string & dns_type, const std::string & credentials);

/// \brief Check if given IP is v4 address
/// \param s IP address string
/// \return Result
bool is_ipv4(const std::string & s);

/// \brief Check if given string is hex string
/// \param s String to test
/// \return Result
bool check_hex(const std::string & s);

/// \brief Check if given IP is v6 address
/// \param s IP address string
/// \return Result
bool is_ipv6(const std::string & s);

/// \brief Try to get IPv4, IPv6 address by parsing output from ip addr command
/// \param result Output from ip addr
/// \param iface Network interface
/// \param ipv4 IPv4 address
/// \param ipv6 IPv6 address
/// \return Result
bool get_ip_from_ip_addr_result(const std::string & result, const std::string & iface,
                                std::string & ipv4, std::string & ipv6);

/// \brief Try to get IPv4, IPv6 address by parsing output from ipconfig command
/// \param result Output from ipconfig
/// \param iface Network interface
/// \param ipv4 IPv4 address
/// \param ipv6 IPv6 address
/// \return Result
bool get_ip_from_ipconfig_result(const std::string & result, const std::string & iface,
                                 std::string & ipv4, std::string & ipv6);

/// \brief HTTP request
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

/// \brief HTTP request with customizable method, e.g. PUT, DELETE...
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

/// \brief Execute shell command with output stored in result
/// \param cmd Shell command
/// \param result Result
/// \return If execution succeeded
bool shell_execute(const std::string & cmd, std::string & result);

#endif //PVE_DDNS_CLIENT_SRC_UTILS_H
