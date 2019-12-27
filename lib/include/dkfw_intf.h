#ifndef _DPDK_FRAMEWORK_INTF_H
#define _DPDK_FRAMEWORK_INTF_H

#define MAX_INTERFACE_NUM 32

#define NETCARD_TYPE_IXGBE 0
#define NETCARD_TYPE_I40E  1

typedef struct _DKFW_INTF_TAG {
    int intf_ind; // 0, 1, 2 ...
    int nic_type;
} DKFW_INTF;

extern int gkfw_interfaces_init(int txq_num, int rxq_num);

#endif

