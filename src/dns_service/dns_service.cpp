#include "dns_service.h"

#include "glog/logging.h"

#include "../utils.h"
#include "dns_service_porkbun.h"
#include "dns_service_dnspod.h"
#include "dns_service_cloudflare.h"

IDnsService * DnsServiceFactory::create(const std::string & service_name)
{
    if (service_name.empty())
    {
        LOG(WARNING) << "Invalid service_name!";
        return nullptr;
    }

    if (str_iequals(service_name, DNS_SERVICE_PORKBUN))
    {
        auto * service = new(std::nothrow) DnsServicePorkbun();
        if (nullptr == service)
        {
            LOG(ERROR) << "Failed to instantiate DnsServicePorkbun!";
            return nullptr;
        }
        return service;
    }

    if (str_iequals(service_name, DNS_SERVICE_DNSPOD))
    {
        auto * service = new(std::nothrow) DnsServiceDnspod();
        if (nullptr == service)
        {
            LOG(ERROR) << "Failed to instantiate DnsServiceDnspod!";
            return nullptr;
        }
        return service;
    }

    if (str_iequals(service_name, DNS_SERVICE_CLOUDFLARE))
    {
        auto * service = new(std::nothrow) DnsServiceCloudflare();
        if (nullptr == service)
        {
            LOG(ERROR) << "Failed to instantiate DnsServiceCloudflare!";
            return nullptr;
        }
        return service;
    }

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
    else if (str_iequals(name, DNS_SERVICE_DNSPOD))
    {
        auto * g = dynamic_cast<DnsServiceDnspod *>(dns_service);
        if (nullptr == g)
            LOG(WARNING) << "dns_service is not instance of DnsServiceDnspod!";
        delete g;
    }
    else if (str_iequals(name, DNS_SERVICE_CLOUDFLARE))
    {
        auto * g = dynamic_cast<DnsServiceCloudflare *>(dns_service);
        if (nullptr == g)
            LOG(WARNING) << "dns_service is not instance of DnsServiceCloudflare";
        delete g;
    }
    else
        LOG(WARNING) << "Unsupported dns service '" << name << "'!";
}
