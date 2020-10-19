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

# 修改grup
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
