#ifndef PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_CLOUDFLARE_H
#define PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_CLOUDFLARE_H

#include <unordered_map>

#include "dns_service.h"

class DnsServiceCloudflare : public IDnsService
{
public:
    const std::string & getServiceName() override;
    bool setCredentials(const std::string & cred_str) override;
    std::string getIpv4(const std::string & domain) override;
    std::string getIpv6(const std::string & domain) override;
    bool setIpv4(const std::string & domain, const std::string & ip) override;
    bool setIpv6(const std::string & domain, const std::string & ip) override;

protected:
    bool verifyToken();
    bool getZoneId(const std::string & domain_name, std::string & out_zone_id);
    bool getRecordId(const std::string & domain_name, const std::string & zone_id, const std::string & type,
                     std::string & out_record_id, std::string & out_record_content);
    std::string getIp(const std::string & domain, bool is_v4);
    bool setIp(const std::string & domain, const std::string & ip, bool is_v4);

private:
    /// Service name
    std::string _service_name = DNS_SERVICE_CLOUDFLARE;
    /// API token
    std::string _token;
    /// Zone ID map
    std::unordered_map<std::string, std::string> _zones;
    /// DNS record ID map
    std::unordered_map<std::string, std::string> _records;
};

#endif //PVE_DDNS_CLIENT_SRC_DNS_SERVICE_DNS_SERVICE_CLOUDFLARE_H
