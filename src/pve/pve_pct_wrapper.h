#ifndef PVE_DDNS_CLIENT_SRC_PVE_PVE_PCT_WRAPPER_H
#define PVE_DDNS_CLIENT_SRC_PVE_PVE_PCT_WRAPPER_H

#include <string>
#include <vector>

/// Proxmox VE pct (Proxmox Container Toolkit) wrapper
class PvePctWrapper
{
public:
    /// Init by executing 'pct list' and update LXC guest VM id list
    /// \return Operation result
    bool init();

    /// Check if given VM id is in LXC guest VM id list
    /// \param vmid VM id
    /// \return Result
    bool isLxcGuest(int vmid) const;

    /// Get LXC guest VM IPv4 and IPv6 address of specific interface
    /// \param vmid VM id
    /// \param iface Interface name
    /// \return A pair of strings, first is IPv4 address, second is IPv6 address (empty string if failed to get)
    std::pair<std::string, std::string> getGuestIp(int vmid, const std::string & iface) const;

protected:
    static bool execute(const std::string & cmd, std::string & result);
    void parseListResult(const std::string & result);

private:
    /// If pct is available
    bool _available = false;
    /// LCX VMID list (from pct list)
    std::vector<int> _lxc_vmids;
};

#endif //PVE_DDNS_CLIENT_SRC_PVE_PVE_PCT_WRAPPER_H
