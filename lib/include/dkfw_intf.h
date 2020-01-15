#ifndef _DPDK_FRAMEWORK_INTF_H
#define _DPDK_FRAMEWORK_INTF_H
#include <rte_mbuf.h>

#define MAX_INTERFACE_NUM 32

#define NETCARD_TYPE_IXGBE 0
#define NETCARD_TYPE_I40E  1

#define MAX_RX_Q_NUM_PER_INTF 16

// 网卡相关数据结构
typedef struct _DKFW_INTF_TAG {
    int intf_seq; // 网卡序号，0,1,2,3
    int nic_type; // 网卡类型，万兆，四万兆

    // 统计信息，从此网卡的rss队列接收到的包数
    unsigned long long stats_rcv_pkts_cnt[MAX_RX_Q_NUM_PER_INTF];
} DKFW_INTF;

extern int interfaces_init(int txq_num, int rxq_num);
extern int dkfw_rcv_pkt_from_interface(int intf_seq, int q_num, struct rte_mbuf **pkts_burst, int max_pkts_num);

#endif

