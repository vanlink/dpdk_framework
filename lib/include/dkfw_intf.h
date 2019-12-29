#ifndef _DPDK_FRAMEWORK_INTF_H
#define _DPDK_FRAMEWORK_INTF_H
#include <rte_mbuf.h>

#define MAX_INTERFACE_NUM 32

#define NETCARD_TYPE_IXGBE 0
#define NETCARD_TYPE_I40E  1

#define MAX_RX_Q_NUM_PER_INTF 16

typedef struct _DKFW_INTF_TAG {
    int intf_ind; // 0, 1, 2 ...
    int nic_type;

    unsigned long long stats_rcv_pkts_cnt[MAX_RX_Q_NUM_PER_INTF];
} DKFW_INTF;

extern int interfaces_init(int txq_num, int rxq_num);
extern int dkfw_rcv_pkt_from_interface(int intf_seq, int q_num, struct rte_mbuf **pkts_burst, int max_pkts_num);

#endif

