# pve-ddns-client
[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause) 
[![CMake](https://github.com/wzkres/pve-ddns-client/actions/workflows/cmake.yml/badge.svg)](https://github.com/wzkres/pve-ddns-client/actions/workflows/cmake.yml)
[![CodeQL](https://github.com/wzkres/pve-ddns-client/actions/workflows/codeql.yml/badge.svg)](https://github.com/wzkres/pve-ddns-client/actions/workflows/codeql.yml)

# English Documentation

A lightweight Dynamic DNS (DDNS) update service written in C++ and designed specifically for Proxmox VE.

## Overview

This application is designed to automatically update DNS records when dynamic IP addresses (DHCP) change for Proxmox VE (hereinafter referred to as **PVE**) hosts and guests.

It is typically deployed on a PVE host system, where it can simultaneously manage DDNS updates for:

* The PVE host itself
* KVM virtual machines
* LXC containers

The application can also be deployed on any device capable of accessing the PVE API. In this scenario, DDNS updates for PVE hosts and KVM guests remain functional, but LXC guest updates are unavailable because the application cannot invoke the `pct` command outside the PVE host.

Alternatively, it can be used as a standard DDNS client on any system by configuring only the `client` section, making it suitable for conventional DDNS use cases on Windows, macOS, Linux, and other platforms.

## Usage

### Complete Configuration File Example (`pve-ddns-client.yml`)

```yaml
# General configuration
general:
  # Update interval in milliseconds (effective only in service mode)
  update-interval-ms: 300000
  # Log retention period in days
  log-overdue-days: 3
  # Log buffering time in seconds
  log-buf-secs: 2
  # Maximum log file size in MB before rotation
  max-log-size-mb: 2
  # Run as a background service
  service-mode: true
  # Public IP detection configuration
  public-ip:
    # Supported services: porkbun, ipify
    service: porkbun
    # Authentication credentials
    # porkbun: api_key,secret_key
    # ipify: no authentication required
    credentials: api_key,secret_key
  # Proxmox VE API configuration
  pve-api:
    # API endpoint
    host: https://pve.domain.com:8006
    # Username
    user: root
    # Authentication realm
    realm: pam
    # API Token ID
    token-id: ddns
    # API Token UUID
    token-uuid: uuid
  # Special feature:
  # Synchronize the host's static IPv6 address with a guest VM's dynamic IPv6 address.
  # Useful when the PVE host cannot obtain an IPv6 address via SLAAC or DHCPv6.
  sync_host_static_v6_address: false
# DDNS configuration for the machine running this application.
# This machine does not have to be a PVE host.
# When only this section is configured, the application behaves like a standard DDNS client,
# obtaining public IPv4/IPv6 addresses through the configured public-ip service.
client:
  # Supported providers: porkbun, dnspod, cloudflare
  dns: dnspod
  # Authentication credentials
  # porkbun: api_key,secret_key
  # dnspod: token_id,token
  # cloudflare: api_token
  credentials: token_id,token
  # IPv4 A records to update
  ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
  # IPv6 AAAA records to update
  ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
# DDNS configuration for the PVE host
# The application retrieves IPv4/IPv6 addresses directly from the specified host interface via the PVE API.
host:
  node: node
  iface: vmbr0
  dns: porkbun
  credentials: api_key,secret_key
  ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
  ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
# DDNS configuration for PVE guest systems
# Similar to the host configuration, with an additional vmid field.
guests:
  # Example: KVM virtual machine
  - node: node
    vmid: 100
    iface: ens18
    dns: porkbun
    credentials: api_key,secret_key
    ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
    ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
  # Example: LXC container
  # Requires the application to be running on the PVE host.
  - node: node
    vmid: 101
    iface: eth0
    dns: porkbun
    credentials: api_key,secret_key
    ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
    ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
```
### Command Line Options
```text
usage: ./pve-ddns-client [options] ...

options:
  -v, --version    Show version information
  -h, --help       Show help information
  -c, --config     Specify configuration file (default: ./pve-ddns-client.yml)
  -l, --log        Specify log directory (default: ./)
```
### Example systemd Service for a PVE Host
Place the following file at:
```text
/lib/systemd/system/pve-ddns-client.service
```
```ini
[Unit]
Description=A Proxmox VE dedicated DDNS updater
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=3
User=root
ExecStart=/root/pve-ddns-client/pve-ddns-client -c /root/pve-ddns-client/pve-ddns-client.yml -l /root/pve-ddns-client/log

[Install]
WantedBy=multi-user.target
```

## Building
Please refer to the GitHub Actions workflow:
https://github.com/wzkres/pve-ddns-client/blob/main/.github/workflows/cmake.yml
To build successfully, ensure that your build environment provides the same compiler toolchain and dependency versions as the GitHub CI environment.


## 中文说明
一款专为Proxmox VE设计，C++编写的轻量型DDNS更新服务程序
### 详细介绍
本程序用于配合Proxmox VE（以下简称PVE）虚拟化环境下的宿主机和客户机动态IP（DHCP）变化，自动更新相关域名记录。一般部署于PVE宿主系统中（可同时支持宿主、KVM客户、LXC客户系统的动态IP域名更新），也可部署在任意可访问到PVE宿主API的设备上（此时由于无法调用宿主系统上的pct命令行工具，所有LXC客户系统将无法正常更新DDNS域名），甚至可以作为普通DDNS更新程序部署在任何设备上（配置文件中仅指定client配置）。
### 使用说明
- pve-ddns-client.yml 完整配置文件说明：
```yaml
# 通用配置部分
general:
  # 更新间隔时间，单位毫秒，仅服务模式时有效
  update-interval-ms: 300000
  # 日志保留时间，单位天
  log-overdue-days: 3
  # 日志缓冲时间，单位秒
  log-buf-secs: 2
  # 日志文件滚动大小，单位兆
  max-log-size-mb: 2
  # 是否作为服务模式启动
  service-mode: true
  # 公网IP获取方式
  public-ip:
    # 服务类型，可选值为 porkbun, ipify
    service: porkbun
    # 服务鉴权信息
    # porkbun为 api_key,secret_key 的格式
    # ipify不需要鉴权
    credentials: api_key,secret_key
  # Proxmox VE API访问相关配置
  pve-api:
    # API访问地址
    host: https://pve.domain.com:8006
    # 用户名
    user: root
    # realm
    realm: pam
    # Token ID
    token-id: ddns
    # Token UUID
    token-uuid: uuid
  # 特殊功能，根据VM的动态IPv6地址，更新宿主系统的静态IPv6地址(适用于PVE宿主无法SLAAC或DHCP获取V6地址的情况)
  sync_host_static_v6_address: false
# 客户端DDNS配置部分（运行本程序的系统，不一定是PVE的宿主，只填写此部分配置时本程序工作方式与普通DDNS更新程序工作方式类似，通过general配置中的public-ip指定的服务获取公网v4、v6地址并更新指定的域名解析记录，可用于如Windows、Mac系统的常规DDNS更新）
client:
  # 服务类型，可选值为 porkbun, dnspod, cloudflare
  dns: dnspod
  # 鉴权信息
  # porkbun为 api_key,secret_key 的格式
  # dnspod为 token_id,token 的格式
  # cloudflare为 api_token 的格式
  credentials: token_id,token
  # 所有需要更新IPv4 A记录的域名
  ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
  # 所有需要更新IPv6 AAAA记录的域名
  ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
# PVE宿主DDNS配置部分（直接通过PVE API获取指定网卡的v4、v6地址用于更新指定的域名解析记录）
host:
  # node名
  node: node
  # 网卡名
  iface: vmbr0
  # 服务类型，参考client部分说明
  dns: porkbun
  # 鉴权信息，参考client部分说明
  credentials: api_key,secret_key
  # 所有需要更新IPv4 A记录的域名
  ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
  # 所有需要更新IPv6 AAAA记录的域名
  ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
# PVE客户虚拟机DDNS配置部分，除需指定vmid外，其它配置项与host一致（运行于PVE宿主系统，直接通过PVE API获取指定虚机的指定网卡的v4、v6地址用于更新指定的域名解析记录）
guests:
  # KVM客户系统节点示例
  - node: node
    vmid: 100
    iface: ens18
    dns: porkbun
    credentials: api_key,secret_key
    ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
    ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
  # LXC客户系统节点示例（此时程序需运行在PVE宿主系统上）
  - node: node
    vmid: 101
    iface: eth0
    dns: porkbun
    credentials: api_key,secret_key
    ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
    ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
```
- 程序参数说明
```
usage: ./pve-ddns-client [options] ... 
options:
  -v, --version    显示版本号
  -h, --help       显示使用说明
  -c, --config     指定配置文件(默认 ./pve-ddns-client.yml)
  -l, --log        指定日志保存位置(默认 ./)
```
- PVE宿主系统systemd服务示例（置于：/lib/systemd/system/pve-ddns-client.service）
```
[Unit]
Description=A Proxmox VE dedicated DDNS updater
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=3
User=root
ExecStart=/root/pve-ddns-client/pve-ddns-client -c /root/pve-ddns-client/pve-ddns-client.yml -l /root/pve-ddns-client/log

[Install]
WantedBy=multi-user.target
```
### 构建方式
请参考GitHub Actions workflow：https://github.com/wzkres/pve-ddns-client/blob/main/.github/workflows/cmake.yml ，需保证编译环境具备与GitHub CI环境一致的编译工具等依赖项
