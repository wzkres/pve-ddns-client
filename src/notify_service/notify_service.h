#ifndef PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_H
#define PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_H

#include <string>


/// Notify service implementations
constexpr const char * NOTIFY_SERVICE_LUA = "lua";

/// Notify service interface
class INotifyService
{
public:
    /// Get service name
    /// \return Service name string
    virtual const std::string & getServiceName() = 0;

    /// Set credentials string (format is implementation dependent)
    /// \param cred_str Credentials string
    /// \return Operation result
    virtual bool setCredentials(const std::string & cred_str) = 0;

    /// \brief Notify when a IP change event occured
    /// \param is_v6 True for v6, false for v4
    /// \param domain Domain name
    /// \param old_ip Old IP
    /// \param new_ip New IP
    /// \return Operation result
    virtual bool notifyIpChange(bool is_v6, const std::string &domain,
                                const std::string &old_ip, const std::string &new_ip) = 0;
};

/// Notify service factory
class NotifyServiceFactory
{
public:
    /// Create notify service instance
    /// \param service_name Service name
    /// \return Instance pointer or nullptr if failed
    static INotifyService * create(const std::string & service_name);

    /// Destroy notify service instance
    /// \param notify_service Instance pointer
    static void destroy(INotifyService * notify_service);
};

#endif //PVE_DDNS_CLIENT_SRC_NOTIFY_SERVICE_NOTIFY_SERVICE_H
