#ifndef PUBLIC_IP_GETTER_IFACE_H
#define PUBLIC_IP_GETTER_IFACE_H

#include "public_ip_getter.h"

/// Public IP getter using local network interface
class PublicIpGetterIface : public IPublicIpGetter
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4() override;
    std::string getIpv6() override;

protected:
    bool getIp(std::string & v4_ip, std::string & v6_ip);

private:
    /// Service name
    std::string _service_name = PUBLIC_IP_GETTER_IFACE;
    /// Network interface
    std::string _interface;
};

#endif //PUBLIC_IP_GETTER_IFACE_H
