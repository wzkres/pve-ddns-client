#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H

#include <string>

/// DNS service implementations
constexpr const char * DNS_SERVICE_PORKBUN = "porkbun";

/// DNS service interface
class IDnsService
{
public:
    /// Get service name
    /// \return Service name string
    virtual const std::string & getServiceName() = 0;

    /// Set credentials string (format is implementation dependent)
    /// \param cred_str Credentials string
    /// \return Operation result
    virtual bool setCredentials(const std::string & cred_str) = 0;

    /// Get IPv4 address of domain (A record)
    /// \param domain Domain name (e.g. sub.site.com)
    /// \return IPv4 address or empty string if failed
    virtual std::string getIpv4(const std::string & domain) = 0;

    /// Get IPv6 address of domain (AAAA record)
    /// \param domain Doamin name (e.g. sub.site.com)
    /// \return IPv6 address or empty string if failed
    virtual std::string getIpv6(const std::string & domain) = 0;

    /// Set IPv4 address of domain (A record)
    /// \param domain Domain name
    /// \param ip IPv4 address string
    /// \return Operation result
    virtual bool setIpv4(const std::string & domain, const std::string & ip) = 0;

    /// Set IPv6 address of dmain (AAAA record)
    /// \param domain Domain name
    /// \param ip IPv6 address string
    /// \return Operation result
    virtual bool setIpv6(const std::string & domain, const std::string & ip) = 0;
};

/// DNS service factory
class DnsServiceFactory
{
public:
    /// Create DNS service instance
    /// \param service_name Service name
    /// \return Instance pointer or nullptr if failed
    static IDnsService * create(const std::string & service_name);

    /// Destroy DNS service instance
    /// \param dns_service Instance pointer
    static void destroy(IDnsService * dns_service);
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_H
