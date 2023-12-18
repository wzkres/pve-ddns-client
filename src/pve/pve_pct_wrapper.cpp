#include "pve_pct_wrapper.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <array>

#include "fmt/format.h"
#include "glog/logging.h"

static const char * pct_cmd = "pct";

#if WIN32
#define pve_popen _popen
#define pve_pclose _pclose
#else
#define pve_popen popen
#define pve_pclose pclose
#endif

bool PvePctWrapper::init()
{
    const std::string pct_list = fmt::format("{} list", pct_cmd);
    std::string result;
    if (!execute(pct_list, result))
    {
        LOG(WARNING) << "Failed to get LCX vmid list, result is '" << result << "'!";
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
    if (!execute(pct_ip_addr, result))
    {
        LOG(WARNING) << "Failed to get ip of LXC guest vmid '" << vmid << "', result is '" << result << "'!";
        return { "", "" };
    }

    std::istringstream f(result);
    std::string line;
    std::string v4_ip, v6_ip;
    std::string iface_cur;
    while (std::getline(f, line))
    {
        if (!v4_ip.empty() && !v6_ip.empty())
            break;

        const char * nptr = line.c_str();
        char * endptr = nullptr;
        strtol(nptr, &endptr, 10);
        if (nptr != endptr)
        {
            std::string::size_type iface_begin = endptr - nptr + 2;
            std::string::size_type iface_end = line.find_first_of(':', iface_begin);
            if (std::string::npos != iface_end)
                iface_cur = line.substr(iface_begin, iface_end - iface_begin);
        }
        else if (iface_cur.find(iface) != std::string::npos)
        {
            std::string::size_type inet6_pos = line.find("inet6 ");
            if (std::string::npos != inet6_pos)
            {
                if (v6_ip.empty())
                {
                    std::string::size_type v6_ip_end = line.find_first_of('/', inet6_pos + 6);
                    if (std::string::npos != v6_ip_end)
                    {
                        v6_ip = line.substr(inet6_pos + 6, v6_ip_end - inet6_pos - 6);
                        if (v6_ip.compare(0, 4, "fe80") == 0)
                            v6_ip.clear();
                    }
                }
                continue;
            }
            std::string::size_type inet_pos = line.find("inet ");
            if (std::string::npos != inet_pos)
            {
                if (v4_ip.empty())
                {
                    std::string::size_type v4_ip_end = line.find_first_of('/', inet_pos + 5);
                    if (std::string::npos != v4_ip_end)
                        v4_ip = line.substr(inet_pos + 5, v4_ip_end - inet_pos - 5);
                }
            }
        }
    }
    return { v4_ip, v6_ip };
}

bool PvePctWrapper::execute(const std::string & cmd, std::string & result)
{
    if (cmd.empty())
    {
        LOG(WARNING) << "Invalid cmd!";
        return false;
    }
    std::array<char, 128> buffer = {};
    FILE * pipe = pve_popen(cmd.c_str(), "r");
    if (nullptr == pipe)
    {
        LOG(WARNING) << "Failed to popen '" << cmd << "'!";
        return false;
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        result += buffer.data();
    const int res = pve_pclose(pipe);
    if (res != 0)
    {
        LOG(WARNING) << "Error pclose result '" << res << "' from execution of '" << cmd << "'!";
        return false;
    }
    return true;
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
        LOG(INFO) << "Got LXC VMID '" << vmid << "' from result line '" << line << "'.";
        _lxc_vmids.emplace_back(vmid);
    }
    LOG(INFO) << "Total " << _lxc_vmids.size() << " LXC VM id(s) parsed from pct list result.";
}
