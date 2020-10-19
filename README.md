# 介绍
基于DPDK的包分发处理工具。
分两部分：

 - lib目录：工具库
 
 - test目录：基于工具库的测试程序

# 编译安装


## 基础环境
linux操作系统，gcc/make等基础编译环境安装。
## 安装DPDK
[安装DPDK](dpdk-install.md)

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

# 测试
## 专用分发核测试
### 测试拓扑
![image](https://github.com/vanlink/dpdk_framework/blob/master/img/test1.png)
如上图所示：
 - 测试仪（公司其他产品，不在此项目中）与交换机两个接口连接
 - nta服务器与交换机一个接口连接（PCI号为0000:01:00.0）
 - 测试仪生成测试流量
 - 配置交换机，将测试仪的流量镜像到nta的接口
 - 测试程序使用一个专用分发核（test-distr0，核37）
 - 测试程序使用两个业务处理核（test-app0，核35，test-app1，核36）
### 测试步骤
 - 运行./test-app0
此为dpdk主进程，必须先运行

等待输出“----- dpdk core_func starts -----”，启动成功

- 运行./test-app1和./test-distr0

等待输出“----- dpdk core_func starts -----”，启动成功

以上进程会定期输出本进程的统计信息：

pkts:累计收包数   pps:每秒收包数    bits:累计接收比特数    bps:接收比特数每秒

- 控制测试仪开始发包

观察测试仪和各进程统计数据

查看测试仪统计数据（在测试仪端查看，单位为“每秒”）：

``` 
ibits         obits      ipackets      opackets 
-----------------------------------------------------------
12880000      13480494         15000         20000 
```
说明测试仪（双向）镜像给nta的：

每秒包数：15000+20000=35000

每秒比特数：12880000+13480494=‭26360494‬

观察test-distr0的输出：
```
pkts:23129538     pps:35000        bits:17419846880     bps:26360000 
```

观察两个test-app的输出：
```
pkts:12710934     pps:17355        bits:9573149696      bps:13071396
pkts:12693604     pps:17644        bits:9560097184      bps:13288603
```

可见两个test-app的速率相关统计之和，等于test-distr0的速率统计，且与测试仪自身统计相符。

## 混合核测试
### 测试拓扑
![image](https://github.com/vanlink/dpdk_framework/blob/master/img/test2.png)
如上图所示：
 - 网卡划分两个RSS队列
 - 测试程序使用两个混合处理核（test-mix0，核35，test-mix1，核36）
 - 每个核内有两个流程，流量分发和流量处理

### 测试步骤
 - 运行./test-mix0
此为dpdk主进程，必须先运行

等待输出“----- dpdk core_func starts -----”，启动成功

- 运行./test-mix1

等待输出“----- dpdk core_func starts -----”，启动成功

以上进程会定期输出本进程的统计信息：

pkts:累计收包数   pps:每秒收包数    bits:累计接收比特数    bps:接收比特数每秒

- 控制测试仪开始发包

观察测试仪和各进程统计数据

查看测试仪统计数据（在测试仪端查看，单位为“每秒”）：

``` 
        ibits         obits      ipackets      opackets
---------------------------------------------------------------
     15456472      16176000         18000         24000
```
说明测试仪（双向）镜像给nta的：

每秒包数：18000+24000=42000

每秒比特数：15456472+16176000=‭31632472‬

观察两个test-mix的输出：
```
pkts:258251       pps:21091        bits:194499896       bps:15884536
pkts:256158       pps:20909        bits:192923568       bps:15747464
```

可见两个test-mix的速率相关统计之和，与测试仪自身统计相符。
