#include "dns_service_porkbun.h"

#include "glog/logging.h"

const std::string & DnsServicePorkbun::getServiceName()
{
    return _service_name;
}

bool DnsServicePorkbun::setCredentials(const std::string & cred_str)
{
    if (cred_str.empty())
    {
        LOG(WARNING) << "Credentials string is empty!";
        return false;
    }
    std::string::size_type comma_pos = cred_str.find(',');
    if (std::string::npos == comma_pos)
    {
        LOG(WARNING) << "Invalid credentials string '" << cred_str << "', should be in format 'API_KEY,API_SECRET'!";
        return false;
    }
    _api_key = cred_str.substr(0, comma_pos);
    _api_secret = cred_str.substr(comma_pos + 1);

    return true;
}
