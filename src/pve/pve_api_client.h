#ifndef PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
#define PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H

#include <string>

/// Proxmox VE API client
class PveApiClient
{
public:
    /// Init using infos from global config
    /// \return Init result
    bool init();

    /// Get host node IPv4 and IPv6 address of specific interface
    /// \param node PVE node name
    /// \param iface Interface name
    /// \return A pair of strings, first is IPv4 address, second is IPv6 address (empty string if failed to get)
    std::pair<std::string, std::string> getHostIp(const std::string & node, const std::string & iface);

    /// Get KVM guest VM IPv4 and IPv6 address of specific interface
    /// \param node PVE node name
    /// \param vmid VM id
    /// \param iface Interface name
    /// \return A pair of strings, first is IPv4 address, second is IPv6 address (empty string if failed to get)
    std::pair<std::string, std::string> getGuestIp(const std::string & node,
                                                   int vmid,
                                                   const std::string & iface);

    /// Set IPv4, IPv6 address of a specific host interface (will only generate a temp modified config file)
    /// \param node PVE node name
    /// \param iface Interface name
    /// \param v4_ip IPv4 address
    /// \param v6_ip IPv6 address
    /// \return Operation result
    bool setHostNetworkAddress(const std::string & node, const std::string & iface,
                               const std::string & v4_ip, const std::string & v6_ip);

    /// Apply temp config file generated by set operation
    /// \param node PVE node name
    /// \return Operation result
    bool applyHostNetworkChange(const std::string & node);

    /// Revert (discard) temp modified config file
    /// \param node PVE node name
    /// \return Operation result
    bool revertHostNetworkChange(const std::string & node);

protected:
    bool req(const std::string & api_url, const std::string & req_data, int & resp_code, std::string & resp_data) const;
    bool reqHostNetwork(const std::string & method, const std::string & node) const;
    bool checkApiHost() const;

private:
    /// PVE API host (root url)
    std::string _api_host;
    /// PVE API access token
    std::string _api_token;
};

#endif //PVE_DDNS_CLIENT_SRC_PVEAPICLIENT_H
