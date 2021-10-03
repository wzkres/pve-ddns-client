#include "pve_pct_wrapper.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>

#include "fmt/format.h"
#include "glog/logging.h"

static const char * pct_cmd = "pct";

bool PvePctWrapper::init()
{
    const std::string pct_list = fmt::format("{} list", pct_cmd);
    std::string result;
    if (!execute(pct_list, result))
    {
        LOG(WARNING) << "Failed to get LCX vmid list, result is '" << result << "'!";
        return false;
    }
    parseListResult(result);
    _available = true;
    return true;
}

bool PvePctWrapper::execute(const std::string & cmd, std::string & result)
{
    if (cmd.empty())
    {
        LOG(WARNING) << "Invalid cmd!";
        return false;
    }
    std::array<char, 128> buffer = {};
    FILE * pipe = popen(cmd.c_str(), "r");
    if (nullptr == pipe)
    {
        LOG(WARNING) << "Failed to popen '" << cmd << "'!";
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();
    const int res = pclose(pipe);
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
    LOG(INFO) << "Total " << _lxc_vmids.size() << " was successfully parsed from pct list result.";
}
