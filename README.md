# pve-ddns-client
[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/wzkres/pve-ddns-client.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/wzkres/pve-ddns-client/context:cpp)
[![CMake](https://github.com/wzkres/pve-ddns-client/actions/workflows/cmake.yml/badge.svg)](https://github.com/wzkres/pve-ddns-client/actions/workflows/cmake.yml)

## EN
A Proxmox VE (PVE) dedicated DDNS updater written in C++
### Detailed description
### Usage
- pve-ddns-client.yml yaml config file

- Command line parameters
```
usage: ./pve-ddns-client [options] ... 
options:
  -v, --version    Print version
  -h, --help       Show usage
  -c, --config     Config yaml file to load (string [=./pve-ddns-client.yml])
  -l, --log        Log file path (string [=./])
```
### Build
Please refer to GitHub Actions workflow：https://github.com/wzkres/pve-ddns-client/blob/main/.github/workflows/cmake.yml

## CN
一款C++开发的针对Proxmox VE环境的DDNS自动更新程序
### 详细介绍
本程序用于配合Proxmox VE（以下简称PVE）虚拟化环境下的宿主机和客户机动态IP（DHCP）变化，自动更新相关域名记录。一般部署于PVE宿主系统中（可同时支持宿主、KVM客户、LXC客户系统的动态IP域名更新），也可部署在任意可访问到PVE宿主API的设备上（此时由于无法调用宿主系统上的pct命令行工具，所有LXC客户系统将无法正常更新DDNS域名），甚至可以作为普通DDNS更新程序部署在任何设备上（配置文件中仅指定client配置）。
### 使用说明
- pve-ddns-client.yml 配置文件说明：
```yaml
# 通用配置
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
  # 特殊功能，根据VM的动态IPv6地址，更新宿主系统的静态IPv6地址
  sync_host_static_v6_address: false
# 客户端DDNS配置（运行本程序的系统，不一定是PVE的宿主，此时本程序工作方式与一般DDNS更新程序类似）
client:
  # 服务类型，可选值为 porkbun, dnspod
  dns: dnspod
  # 鉴权信息
  # porkbun为 api_key,secret_key 的格式
  # dnspod为 token_id,token 的格式
  credentials: token_id,token
  # 所有需要更新IPv4 A记录的域名
  ipv4: ["v4sub1.domain.com", "v4sub2.domain.com"]
  # 所有需要更新IPv6 AAAA记录的域名
  ipv6: ["v6sub1.domain.com", "v6sub2.domain.com"]
# PVE宿主DDNS配置
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
# PVE客户虚拟机DDNS配置，除需指定vmid外，其它配置项与host一致
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
### 构建
请参考GitHub Actions workflow：https://github.com/wzkres/pve-ddns-client/blob/main/.github/workflows/cmake.yml