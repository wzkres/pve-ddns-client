#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_DNSPOD_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_DNSPOD_H

#include <vector>

#include "dns_service.h"

/// DNSPod domain record cache
typedef struct dnspod_record_cache_
{
    std::string domain;
    bool is_v4;
    std::string record_id;
    std::string line_id;
} dnspod_record_cache;

/// DNSPod tencent DNS service implementation
class DnsServiceDnspod : public IDnsService
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4(const std::string & domain) override;
    std::string getIpv6(const std::string & domain) override;
    bool setIpv4(const std::string & domain, const std::string & ip) override;
    bool setIpv6(const std::string & domain, const std::string & ip) override;

protected:
    bool getVersion(std::string & version);
    std::string getIp(const std::string & domain, bool is_v4);
    bool setIp(const std::string & domain, const std::string & ip, bool is_v4);
    bool updateRecordCache(const std::string & domain, bool is_v4,
                           const std::string & record_id, const std::string & line_id);
    const dnspod_record_cache * getRecordCache(const std::string & domain, bool is_v4) const;

private:
    /// Service name
    std::string _service_name = DNS_SERVICE_DNSPOD;
    /// DNSPod token (id,token)
    std::string _token;
    /// Domain records cache
    std::vector<dnspod_record_cache> _records_cache;
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_DNSPOD_H
