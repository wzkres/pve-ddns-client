#include "dns_service.h"

#include "glog/logging.h"

#include "../utils.h"
#include "dns_service_porkbun.h"

IDnsService * DnsServiceFactory::create(const std::string & service_name)
{
    if (str_iequals(service_name, DNS_SERVICE_PORKBUN))
    {
        auto * getter = new(std::nothrow) DnsServicePorkbun();
        if (nullptr == getter)
        {
            LOG(ERROR) << "Failed to instantiate DnsServicePorkbun!";
            return nullptr;
        }
        return getter;
    }
    else
        LOG(WARNING) << "Unsupported dns service '" << service_name << "'!";

    return nullptr;
}

void DnsServiceFactory::destroy(IDnsService * dns_service)
{
    if (nullptr == dns_service)
    {
        LOG(WARNING) << "Invalid param!";
        return;
    }

    const std::string & name = dns_service->getServiceName();
    if (str_iequals(name, DNS_SERVICE_PORKBUN))
    {
        auto * g = dynamic_cast<DnsServicePorkbun *>(dns_service);
        if (nullptr == g)
            LOG(WARNING) << "dns_service is not instance of DnsServicePorkbun!";
        delete g;
    }
    else
        LOG(WARNING) << "Unsupported dns service '" << name << "'!";
}