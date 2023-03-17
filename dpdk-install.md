# 安装编译工具
```shell
dnf update --nobest
dnf install pkg-config
dnf groupinstall "Development Tools"
dnf --enablerepo=PowerTools install pciutils openssl-devel bc numactl numactl-devel \ 
                                    python3 libpcap-devel samba lrzsz vim meson ninja-build
pip3 install pyelftools
dnf install epel-release
dnf --enablerepo=PowerTools install cjson cjson-devel
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

## 设置fpic
```shell
export EXTRA_CFLAGS=-fPIC
```
## 正式编译

```shell
meson setup build
cd build
ninja
ninja install
```

编译后：
```shell
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
echo "/usr/local/lib64" > /etc/ld.so.conf.d/dpdk.conf
ldconfig
ldconfig -p | grep librte
```

## 开启IOMMU
BIOS开启虚拟化iommu

grub添加：
```shell
iommu=pt intel_iommu=on
```
之后执行
```shell
grub2-mkconfig -o /boot/grub2/grub.cfg
```
重启

执行
```shell
modprobe vfio-pci
```
之后可绑定dpdk网卡
# 快捷方式

cat prompt.sh
```shell
#!/bin/bash
export PS1="\[\e[1m\]\[\e[32m\][\w]# \[\e[m"
```
```shell
echo 'export PS1="\[\e[1m\]\[\e[32m\][\w]# \[\e[m"' > /etc/profile.d/promptfullpath.sh
```

/usr/src/dpdksetup.sh
```shell
service iptables stop
systemctl stop iptables.service
systemctl stop firewalld.service
systemctl disable firewalld.service
mount -t hugetlbfs nodev /mnt/huge
echo 0 > /proc/sys/kernel/randomize_va_space

modprobe vfio-pci

python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=vfio-pci 0000:01:00.0
python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=vfio-pci 0000:01:00.1

python ${RTE_SDK}/usertools/dpdk-devbind.py --status
```
