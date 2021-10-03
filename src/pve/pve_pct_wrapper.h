#ifndef PVE_DDNS_CLIENT_SRC_PVE_PVE_PCT_WRAPPER_H
#define PVE_DDNS_CLIENT_SRC_PVE_PVE_PCT_WRAPPER_H

#include <string>
#include <vector>

/// Proxmox VE pct (Proxmox Container Toolkit) wrapper
class PvePctWrapper
{
public:
    bool init();

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
