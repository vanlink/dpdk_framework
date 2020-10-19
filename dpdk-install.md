# 关闭防火墙
```shell
systemctl stop firewalld.service
systemctl disable firewalld.service
```
# 关闭 selinux
略

# 安装编译工具
```shell
yum -y update
yum groupinstall "Development Tools"
yum install -y pciutils gcc openssl-devel bc numactl-devel python libpcap-devel samba lrzsz vim

yum install epel-release
sudo yum -y install gcc libpcap-devel pcre-devel libyaml-devel file-devel \
  zlib-devel jansson-devel nss-devel libcap-ng-devel libnet-devel tar make \
  libnetfilter_queue-devel lua-devel PyYAML libmaxminddb-devel rustc cargo \
  lz4-devel
```

# 修改grub
修改 /etc/default/grub 文件：
```shell
default_hugepagesz=1G hugepagesz=1G hugepages=32 isolcpus=1-7 nohz_full=1-7 rcu_nocbs=1-7 transparent_hugepage=never
```
之后执行
```shell
grub2-mkconfig -o /boot/grub2/grub.cfg
```
重启

# 安装内核dev
更新完内核版本后，必须重启再执行：
```shell
yum install -y kernel-devel-$(uname -r)
```
# 编译安装 DPDK
## 虚拟机e1000网卡
如果使用虚拟机e1000网卡，进行以下修改：

igb_uio.c 文件：
```shell
if (pci_intx_mask_supported(dev)) ...
改为
if (1) ...
```
## 设置fpic
```shell
export EXTRA_CFLAGS=-fPIC
```
## 正式编译
略

# 快捷方式
cat prompt.sh
```shell
#!/bin/bash
export PS1="\[\e[1m\]\[\e[32m\][\w]# \[\e[m"
```
/usr/src/dpdkenv.sh
```shell
export RTE_SDK=/usr/src/dpdk-19.11
export RTE_TARGET=x86_64-native-linuxapp-gcc
export DKFW_SDK=/usr/src/dpdk-frame-new
export NTA_SDK=/usr/src/kafka_api_demo
```

/usr/src/dpdksetup.sh
```shell
service iptables stop
systemctl stop iptables.service
systemctl start smb.service
mount -t hugetlbfs nodev /mnt/huge
echo 0 > /proc/sys/kernel/randomize_va_space

modprobe uio

insmod ${RTE_SDK}/${RTE_TARGET}/kmod/igb_uio.ko
insmod ${RTE_SDK}/${RTE_TARGET}/kmod/rte_kni.ko

python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio 0000:01:00.0
python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio 0000:01:00.1

python ${RTE_SDK}/usertools/dpdk-devbind.py --status
```
