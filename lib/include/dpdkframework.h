#ifndef _DPDK_FRAMEWORK_H
#define _DPDK_FRAMEWORK_H

#include "dkfw_config.h"

// 各个核心数量，网卡数量
extern int g_pkt_process_core_num;
extern int g_pkt_distribute_core_num;
extern int g_other_core_num;

int g_dkfw_interfaces_num;

extern int dkfw_init(DKFW_CONFIG *config);
extern void dkfw_exit(void);
extern int dkfw_start_loop_raw(void *loop_arg);
#endif

