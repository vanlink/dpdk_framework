#ifndef _DPDK_FRAMEWORK_CORE_H
#define _DPDK_FRAMEWORK_CORE_H
#include "dkfw_config.h"
#include <rte_ring.h>

typedef struct _DKFW_CORE_TAG {
    int core_seq;  // 0, 1, 2 ...
    int core_ind;  // logic core ind, in cpu_layout.py
    int core_role;
    int core_is_me;

    int pkts_to_me_q_num;
    struct rte_ring *pkts_to_me_q[MAX_CORES_PER_ROLE];

    lcore_function_t *core_func_raw;
} DKFW_CORE;

extern int cores_init(DKFW_CONFIG *config);

#endif

