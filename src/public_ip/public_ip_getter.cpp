#include "public_ip_getter.h"

#include "glog/logging.h"

#include "../utils.h"
#include "public_ip_getter_porkbun.h"

IPublicIpGetter * PublicIpGetterFactory::create(const std::string & service_name)
{
    if (str_iequals(service_name, PUBLIC_IP_GETTER_PORKBUN))
    {
        auto * getter = new(std::nothrow) PublicIpGetterPorkbun();
        if (nullptr == getter)
        {
            LOG(ERROR) << "Failed to instantiate PublicIpGetterPorkbun!";
            return nullptr;
        }
        return getter;
    }
    else
        LOG(WARNING) << "Unsupported public ip getter '" << service_name << "'!";

    return nullptr;
}

void PublicIpGetterFactory::destroy(IPublicIpGetter * ip_getter)
{
    if (nullptr == ip_getter)
    {
        LOG(WARNING) << "Invalid param!";
        return;
    }

    const std::string & name = ip_getter->getServiceName();
    if (str_iequals(name, PUBLIC_IP_GETTER_PORKBUN))
    {
        auto * g = dynamic_cast<PublicIpGetterPorkbun *>(ip_getter);
        if (nullptr == g)
            LOG(WARNING) << "ip_getter is not instance of PublicIpGetterPorkbun!";
        delete g;
    }
    else
        LOG(WARNING) << "Unsupported public ip getter '" << name << "'!";
}
