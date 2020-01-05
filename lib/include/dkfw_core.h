#ifndef _DPDK_FRAMEWORK_CORE_H
#define _DPDK_FRAMEWORK_CORE_H
#include "dkfw_config.h"
#include <rte_ring.h>
#include <rte_mbuf.h>

typedef struct _DKFW_RING_TAG {
    struct rte_ring *dkfw_ring;
    
    unsigned long long stats_enq_cnt;
    unsigned long long stats_enq_err_cnt;

    unsigned long long stats_deq_cnt;
} DKFW_RING;

typedef struct _DKFW_CORE_TAG {
    int core_seq;  // 0, 1, 2 ...
    int core_ind;  // logic core ind, in cpu_layout.py
    int core_role;
    int core_is_me;

    int pkts_to_me_q_num;
    DKFW_RING pkts_to_me_q[MAX_CORES_PER_ROLE];

    lcore_function_t *core_func_raw;

    struct rte_ring *ipc_to_me;
    struct rte_ring *ipc_to_back;
} DKFW_CORE;

extern int cores_init(DKFW_CONFIG *config);
extern int dkfw_send_pkt_to_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf *mbuf);
extern int dkfw_rcv_pkt_from_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf **pkts_burst, int max_pkts_num);

#endif

