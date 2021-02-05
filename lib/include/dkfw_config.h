#ifndef _DPDK_FRAMEWORK_CONFIG_H
#define _DPDK_FRAMEWORK_CONFIG_H

#include <rte_launch.h>

#define DKFW_STATS_ENABLED   1

#define MAX_JUMBO_FRAME_SIZE 9500

/* 本框架配置参数的数据结构，用于初始化时在 dkfw_init 函数传入 */

enum {
    CORE_ROLE_PKT_PROCESS,     // 核类型：业务核
    CORE_ROLE_PKT_DISPATCH,    // 核类型：分发核
    CORE_ROLE_OTHER,           // 核类型：其他核（暂时未使用）
    CORE_ROLE_MAX,
};

enum {
    PROCESS_TYPE_PRIMARY,      // dpdk主线程（primary）
    PROCESS_TYPE_SECONDARY,    // dpdk从县城（secondary）
};

// 一个核心的配置参数
typedef struct _CORE_CONFIG_TAG {
    int core_enabled;   // 表示此核心使能，需用户设置
    int core_seq;       // 序列号0,1,2... 初始化时自动填入
    int core_ind;       // 此核心的id号，需用户设置，可用dpdk的工具cpu_layout.py查看
    int core_role;      // 类型，CORE_ROLE_*，初始化时自动填入
    int core_is_me;     // 设置为1表示调用 dkfw_init 的进程所在的核心
    
    lcore_function_t *core_func_raw;    // dpdk运行的主循环函数
} CORE_CONFIG;

// 网卡pci相关设置
typedef struct _PCI_CONFIG_TAG {
    char pci_name[64];   // 网卡的pci编号

    int nic_hw_strip_vlan;
    int nic_hw_strip_qinq;

    int nic_symmetric_rss_enable;
    int nic_promiscuous_enable;
    int nic_allmulticast_enable;

    int nic_tx_desc;
    int nic_rx_desc;

} PCI_CONFIG;

#define MAX_CORES_PER_ROLE 32    // 每种核类型的最大使用核心数
#define MAX_PCI_NUM 8            // 最多连接网卡数

// 主配置结构，用户填好此结构，调用 dkfw_init
typedef struct _DKFW_CONFIG_TAG {
    char nuique_name[64];       // 多实例时的唯一标识，一般为空即可

    int single_process;         // single process mode

    int process_type;           // dpdk主/从进程，第一个运行的必须为主进程，其他为从

    int number_of_channels;     //    -n <number of channels>  Set the number of memory channels to use.

    char socket_limit[64];     // --socket-limit <amounts of memory per socket>
                               //Place a per-socket upper limit on memory use (non-legacy memory mode only). 0 will disable the limit for a particular socket.

    int cores_pkt_process_num;   // 业务处理核实际数量，初始化时自动填入
    CORE_CONFIG cores_pkt_process[MAX_CORES_PER_ROLE];   // 业务核具体配置

    int cores_pkt_dispatch_num;  // 分发核实际数量，初始化时自动填入
    CORE_CONFIG cores_pkt_dispatch[MAX_CORES_PER_ROLE];  // 分发核具体配置

    int cores_other_num;
    CORE_CONFIG cores_other[MAX_CORES_PER_ROLE];

    int nic_rx_pktbuf_cnt; // packets cnt for all interfaces
    int nic_max_rx_pkt_len;
    PCI_CONFIG pcis_config[MAX_PCI_NUM];    // 连接网卡具体配置，连接几个网卡就填几个

} DKFW_CONFIG;

#endif

