#include "pve_pct_wrapper.h"

#include <sstream>
#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include "../utils.h"

static const char * pct_cmd = "pct";

bool PvePctWrapper::init()
{
    const std::string pct_list = fmt::format("{} list", pct_cmd);
    std::string result;
    if (!shell_execute(pct_list, result))
    {
        SPDLOG_WARN("Failed to get LCX vmid list, result is '{}'!", result);
        _available = false;
        return false;
    }
    parseListResult(result);
    _available = true;
    return true;
}

bool PvePctWrapper::isLxcGuest(const int vmid) const
{
    if (!_available)
        return false;
    return std::find(_lxc_vmids.begin(), _lxc_vmids.end(), vmid) != _lxc_vmids.end();
}

std::pair<std::string, std::string> PvePctWrapper::getGuestIp(const int vmid, const std::string & iface) const
{
    if (!_available)
        return { "", "" };

    const std::string pct_ip_addr = fmt::format("{} exec {} ip addr", pct_cmd, vmid);
    std::string result;
    if (!shell_execute(pct_ip_addr, result))
    {
        SPDLOG_WARN("Failed to get ip of LXC guest vmid '{}', result is '{}'!", vmid, result);
        return { "", "" };
    }
    std::string v4_ip, v6_ip;
    if (!get_ip_from_ip_addr_result(result, iface, v4_ip, v6_ip))
    {
        SPDLOG_WARN("Failed to get_ip_from_ip_addr_result '{}' with specified iface: {}!", result, iface);
        return { "", "" };
    }
    return { v4_ip, v6_ip };
}

void PvePctWrapper::parseListResult(const std::string & result)
{
    _lxc_vmids.clear();

    std::istringstream f(result);
    std::string line;
    while (std::getline(f, line))
    {
        const char * nptr = line.c_str();
        char * endptr = nullptr;
        const int vmid = static_cast<int>(strtol(line.c_str(), &endptr, 10));
        if (nptr == endptr)
            continue;
        SPDLOG_INFO("Got LXC VMID '{}' from result line '{}'.", vmid, line);
        _lxc_vmids.emplace_back(vmid);
    }
    SPDLOG_INFO("Total {} LXC VM id(s) parsed from pct list result.", _lxc_vmids.size());
}
