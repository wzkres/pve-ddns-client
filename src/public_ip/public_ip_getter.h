#ifndef PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H
#define PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H

#include <string>

/// Public IP getter implementations
constexpr const char * PUBLIC_IP_GETTER_PORKBUN = "porkbun";
constexpr const char * PUBLIC_IP_GETTER_IPIFY = "ipify";

/// Public IP getter interface
class IPublicIpGetter
{
public:
    /// Get service name
    /// \return Service name string
    virtual const std::string & getServiceName() = 0;

    /// Set credentials string (format is implementation dependent)
    /// \param cred_str Credentials string
    /// \return Operation result
    virtual bool setCredentials(const std::string & cred_str) = 0;

    /// Get public IPv4 address
    /// \return IPv4 address or empty string if failed
    virtual std::string getIpv4() = 0;

    /// Get public IPv6 address
    /// \return IPv6 address or empty string if failed
    virtual std::string getIpv6() = 0;
};

/// Public IP getter factory
class PublicIpGetterFactory
{
public:
    /// Create public IP getter instance
    /// \param service_name Service name
    /// \return Instance pointer or nullptr if failed
    static IPublicIpGetter * create(const std::string & service_name);

    /// Destroy public IP getter instance
    /// \param ip_getter Instance pointer
    static void destroy(IPublicIpGetter * ip_getter);
};

#endif //PVE_DDNS_CLIENT_SRC_PUBLIC_IP_PUBLIC_IP_GETTER_H
