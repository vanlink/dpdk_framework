#ifndef _DPDK_FRAMEWORK_CORE_H
#define _DPDK_FRAMEWORK_CORE_H
#include "dkfw_config.h"
#include <rte_ring.h>
#include <rte_mbuf.h>

// 数据包接收数列
typedef struct _DKFW_RING_TAG {
    struct rte_ring *dkfw_ring;  // dpdk 提供的无锁队列
    
    unsigned long long stats_enq_cnt;  // 统计信息，入队列成功次数
    unsigned long long stats_enq_retry_cnt;  // 统计信息，入队列重试次数
    unsigned long long stats_enq_err_cnt;  // 统计信息，入队列失败次数

    unsigned long long stats_deq_cnt;   // 统计信息，出队列次数
} DKFW_RING;

// 一个核心的参数
typedef struct _DKFW_CORE_TAG {
    int core_seq;  // 同CORE_CONFIG
    int core_ind;  // 同CORE_CONFIG
    int core_role; // 同CORE_CONFIG
    int core_is_me; // 同CORE_CONFIG

    // for app cores
    int pkts_to_me_q_num; // 本核心接收数据包的队列数量，每一个队列对应一个分发核
                          // 也就是说，分发核n会把包发送到队列n
    DKFW_RING pkts_to_me_q[MAX_CORES_PER_ROLE];  // 本核心接收数据包的队列

    // for other cores
    int data_to_me_q_num;
    DKFW_RING data_to_me_q[MAX_CORES_PER_ROLE];

    lcore_function_t *core_func_raw; // 同CORE_CONFIG

    struct rte_ring *ipc_to_me;    // 与控制程序通讯接口，后续使用
    struct rte_ring *ipc_to_back;  // 与控制程序通讯接口，后续使用
} DKFW_CORE;

extern int cores_init(DKFW_CONFIG *config);
extern int dkfw_send_pkt_to_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf *mbuf);
extern int dkfw_rcv_pkt_from_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf **pkts_burst, int max_pkts_num);
extern int dkfw_send_data_to_other_core_q(int core_seq, int core_q_num, void *data);
extern int dkfw_rcv_data_from_other_core_q(int core_seq, int core_q_num, void **data_burst, int max_data_num);
extern int dkfw_data_to_other_core_q_stat(int core_seq, int core_q_num, uint64_t *errcnt, uint64_t *used);
extern int dkfw_pkt_to_process_core_q_stat(int core_seq, int core_q_num, uint64_t *errcnt, uint64_t *used);

#endif

