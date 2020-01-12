# 介绍
基于DPDK的包分发处理工具。
分两部分：

 - lib目录：工具库
 
 - test目录：基于工具库的测试程序

# 编译安装
## 安装DPDK
按照dpdk官方文档提供的方式正常安装即可
安装后设置好dpdk相关的环境变量，例如：
```shell
export RTE_SDK=/usr/src/dpdk-19.11
export RTE_TARGET=x86_64-native-linuxapp-gcc
```
设置好dpdk接管的接口，例如：
```shell
python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio 0000:01:00.0
python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio 0000:01:00.1
```
## 编译工具库

- 进入lib目录编译源代码

```shell
cd lib
make
```
编译成功生成 libdkfw.a 静态库文件

- 设置环境变量

设置环境变量为此lib目录，为编译测试程序或其他使用该库的程序做准备。
例如：
```shell
export DKFW_SDK=/usr/src/dpdk_framework/lib
```

## 编译测试程序
- 进入test目录编译源代码
 ```shell
cd test
make
```
编译成功生成test-\*测试程序。
