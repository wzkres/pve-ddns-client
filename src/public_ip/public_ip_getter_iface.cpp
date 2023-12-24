#include "public_ip_getter_iface.h"

#include "glog/logging.h"

#include "../utils.h"


const std::string& PublicIpGetterIface::getServiceName()
{
    return _service_name;
}

bool PublicIpGetterIface::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        LOG(WARNING) << "Credentials string is empty!";
        return false;
    }
    _interface = cred_str;

    return true;
}

std::string PublicIpGetterIface::getIpv4()
{
    std::string ipv4, ipv6;
    if (!getIp(ipv4, ipv6))
    {
        LOG(WARNING) << "Failed to getIp from iface " << _interface << "!";
        return {};
    }
    return ipv4;
}

std::string PublicIpGetterIface::getIpv6()
{
    std::string ipv4, ipv6;
    if (!getIp(ipv4, ipv6))
    {
        LOG(WARNING) << "Failed to getIp from iface " << _interface << "!";
        return {};
    }
    return ipv6;
}

bool PublicIpGetterIface::getIp(std::string& v4_ip, std::string& v6_ip)
{
    std::string result;
#if WIN32
    if (!shell_execute("ipconfig", result))
    {
        LOG(WARNING) << "Failed to shell_execute ipconfig!";
        return false;
    }
    if (!get_ip_from_ipconfig_result(result, _interface, v4_ip, v6_ip))
    {
        LOG(WARNING) << "Failed to get_ip_from_ipconfig_result interface '" << _interface << "' result '"
                     << result << "'!";
        return false;
    }
#else
    if (!shell_execute("ip addr", result))
    {
        LOG(WARNING) << "Failed to shell_execute ip addr!";
        return false;
    }
    if (!get_ip_from_ip_addr_result(result, _interface, v4_ip, v6_ip))
    {
        LOG(WARNING) << "Failed to get_ip_from_ip_addr_result interface '" << _interface << "' result '"
                     << result << "'!";
        return false;
    }
#endif
    return true;
}
