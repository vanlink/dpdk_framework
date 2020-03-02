#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_string_fns.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#include "dkfw_core.h"
#include "dkfw_ipc.h"

int g_pkt_process_core_num = 0;
static DKFW_CORE g_pkt_process_core[MAX_CORES_PER_ROLE];

int g_pkt_distribute_core_num = 0;
static DKFW_CORE g_pkt_distribute_core[MAX_CORES_PER_ROLE];

int g_other_core_num = 0;
static DKFW_CORE g_other_core[MAX_CORES_PER_ROLE];

static DKFW_CORE *g_core_me = NULL;

extern struct rte_ring *ipc_create_or_lookup_ring(int core_role, int core_seq, int ring_type);

/* 控制通讯相关，后续使用 */
static int core_init_ipc_rings(DKFW_CORE *core)
{
    core->ipc_to_me = ipc_create_or_lookup_ring(core->core_role, core->core_seq, IPC_TO_CORE_RING);
    if(!core->ipc_to_me){
        printf("init ipc ring err.\n");
        return -1;
    }
    core->ipc_to_back = ipc_create_or_lookup_ring(core->core_role, core->core_seq, IPC_TO_BACK_RING);
    if(!core->ipc_to_back){
        printf("init ipc back ring err.\n");
        return -1;
    }

    return 0;
}

/*
    初始化一个业务处理核
    to_me_q_num为本核接收包的队列数
    返回0成功，其他失败
*/
static int process_core_init_one(DKFW_CORE *core, int to_me_q_num)
{
    int i;
    char buff[128];
    
    printf("Init process core ind %d: to_me_q_num %d.\n", core->core_ind, to_me_q_num);
    core->pkts_to_me_q_num = to_me_q_num;

    // 初始化每个接收包队列
    for(i=0;i<to_me_q_num;i++){
        snprintf(buff, sizeof(buff), "pctome%d-%d", core->core_ind, i);

        // dpdk主进程创建队列，其他进程查找队列
        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            core->pkts_to_me_q[i].dkfw_ring = rte_ring_create(buff, 65536 * 2, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
        } else {
            core->pkts_to_me_q[i].dkfw_ring = rte_ring_lookup(buff);
        }
        if(!core->pkts_to_me_q[i].dkfw_ring){
            printf("process_core_init_one %s err.\n", buff);
            return -1;
        }
    }

    /* 控制通讯相关，后续使用 */
    if(core_init_ipc_rings(core) < 0){
        return -1;
    }

    return 0;
}

/*
    初始化一个分发核
    返回0成功，其他失败
*/
static int dispatch_core_init_one(DKFW_CORE *core)
{
    printf("Init dispatch core ind %d\n", core->core_ind);

    /* 控制通讯相关，后续使用 */
    if(core_init_ipc_rings(core) < 0){
        return -1;
    }
    
    return 0;
}

/*
    初始化一个其他核
    返回0成功，其他失败
*/
static int other_core_init_one(DKFW_CORE *core, int to_me_q_num)
{
    int i;
    char buff[128];
    
    printf("Init other core ind %d: to_me_q_num %d.\n", core->core_ind, to_me_q_num);
    core->data_to_me_q_num = to_me_q_num;

    for(i=0;i<to_me_q_num;i++){
        snprintf(buff, sizeof(buff), "dttome%d-%d", core->core_ind, i);

        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            core->data_to_me_q[i].dkfw_ring = rte_ring_create(buff, 4096, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
        } else {
            core->data_to_me_q[i].dkfw_ring = rte_ring_lookup(buff);
        }
        if(!core->data_to_me_q[i].dkfw_ring){
            printf("other_core_init_one %s err.\n", buff);
            return -1;
        }
    }

    /* 控制通讯相关，后续使用 */
    if(core_init_ipc_rings(core) < 0){
        return -1;
    }
    
    return 0;
}

/*
    初始化各个核心
    返回0成功，其他失败
*/
int cores_init(DKFW_CONFIG *config)
{
    int i;
    int pkt_to_me_q_num;
    CORE_CONFIG *core_config_curr;
    DKFW_CORE *core_curr;
    
    memset(g_pkt_process_core, 0, sizeof(g_pkt_process_core));
    memset(g_pkt_distribute_core, 0, sizeof(g_pkt_distribute_core));
    memset(g_other_core, 0, sizeof(g_other_core));

    // 初始化相关数据结构
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core_config_curr = &config->cores_pkt_process[i];
        core_curr = &g_pkt_process_core[i];
        if(!core_config_curr->core_enabled){
            break;
        }
        g_pkt_process_core_num++;
        core_curr->core_seq = i;
        core_curr->core_ind = core_config_curr->core_ind;
        core_curr->core_role = core_config_curr->core_role;
        core_curr->core_is_me = core_config_curr->core_is_me;
        if(core_curr->core_is_me){
            g_core_me = core_curr;
        }
        core_curr->core_func_raw = core_config_curr->core_func_raw;
    }
    printf("We have process cores: %d\n", g_pkt_process_core_num);

    // 初始化相关数据结构
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core_config_curr = &config->cores_pkt_dispatch[i];
        core_curr = &g_pkt_distribute_core[i];
        if(!core_config_curr->core_enabled){
            break;
        }
        g_pkt_distribute_core_num++;
        core_curr->core_seq = i;
        core_curr->core_ind = core_config_curr->core_ind;
        core_curr->core_role = core_config_curr->core_role;
        core_curr->core_is_me = core_config_curr->core_is_me;
        if(core_curr->core_is_me){
            g_core_me = core_curr;
        }
        core_curr->core_func_raw = core_config_curr->core_func_raw;
    }
    printf("We have distribute cores: %d\n", g_pkt_distribute_core_num);

    // 初始化相关数据结构
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core_config_curr = &config->cores_other[i];
        core_curr = &g_other_core[i];
        if(!core_config_curr->core_enabled){
            break;
        }
        g_other_core_num++;
        core_curr->core_seq = i;
        core_curr->core_ind = core_config_curr->core_ind;
        core_curr->core_role = core_config_curr->core_role;
        core_curr->core_is_me = core_config_curr->core_is_me;
        if(core_curr->core_is_me){
            g_core_me = core_curr;
        }
        core_curr->core_func_raw = core_config_curr->core_func_raw;
    }
    printf("We have other cores: %d\n", g_other_core_num);

    if(!g_core_me){
        printf("Failed to find me core.\n");
        return -1;
    }

    // 每个业务核收包队列的数量
    if(g_pkt_distribute_core_num){
        // 如果有专用分发核，则业务核收包队列数量设为分发核数，每个分发核对应一个业务核收包队列
        pkt_to_me_q_num = g_pkt_distribute_core_num;
    }else{
        // 如果无有专用分发核，则每个业务核都可能从其他业务核收包
        // 业务核收包队列的数量设为业务核数
        pkt_to_me_q_num = g_pkt_process_core_num;
    }

    // 分别初始化各个核
    for(i=0;i<g_pkt_process_core_num;i++){
        if(process_core_init_one(&g_pkt_process_core[i], pkt_to_me_q_num) < 0){
            return -1;
        }
    }

    // 分别初始化各个核
    for(i=0;i<g_pkt_distribute_core_num;i++){
        if(dispatch_core_init_one(&g_pkt_distribute_core[i]) < 0){
            return -1;
        }
    }

    // 分别初始化各个核
    for(i=0;i<g_other_core_num;i++){
        if(other_core_init_one(&g_other_core[i], g_pkt_process_core_num) < 0){
            return -1;
        }
    }

    return 0;
}

/*
    启动 dpdk 主循环函数
    返回0成功，其他失败
*/
int dkfw_start_loop_raw(void *loop_arg)
{
    if(!g_core_me->core_func_raw){
        return -1;
    }

    rte_eal_mp_remote_launch(g_core_me->core_func_raw, loop_arg, CALL_MASTER);
    rte_eal_mp_wait_lcore();

    return 0;
}

/*
    将一个数据包发送到序号为process_core_seq的业务核的第core_q_num个接收队列
    返回0成功，其他失败
*/
int dkfw_send_pkt_to_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf *mbuf)
{
    DKFW_RING *ring = &g_pkt_process_core[process_core_seq].pkts_to_me_q[core_q_num];

    if(likely(rte_ring_sp_enqueue(ring->dkfw_ring, mbuf) == 0)){
        ring->stats_enq_cnt++;
        return 0;
    }

    ring->stats_enq_err_cnt++;

    return -1;
}

// 发送到每个业务处理核的第q_num个队列的包数
void dkfw_pkt_send_to_process_cores_stat(int q_num, uint64_t *stats, uint64_t *stats_err)
{
    int i;
    DKFW_RING *ring;

    for(i=0;i<g_pkt_process_core_num;i++){
        ring = &g_pkt_process_core[i].pkts_to_me_q[q_num];
        stats[i] = ring->stats_enq_cnt;
        stats_err[i] = ring->stats_enq_err_cnt;
    }
}


/*
    从序号为process_core_seq的业务核的第core_q_num个接收队列中接收数据包
    最多接收max_pkts_num个，pkts_burst为缓冲区
    返回实际接收到的包数
*/
int dkfw_rcv_pkt_from_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf **pkts_burst, int max_pkts_num)
{
    DKFW_RING *ring = &g_pkt_process_core[process_core_seq].pkts_to_me_q[core_q_num];
    int nb_rx = rte_ring_sc_dequeue_burst(ring->dkfw_ring, (void **)pkts_burst, max_pkts_num, NULL);

    ring->stats_deq_cnt += nb_rx;

    return nb_rx;
}

int dkfw_send_data_to_other_core_q(int core_seq, int core_q_num, void *data)
{
    DKFW_RING *ring = &g_other_core[core_seq].data_to_me_q[core_q_num];

    if(likely(rte_ring_sp_enqueue(ring->dkfw_ring, data) == 0)){
        ring->stats_enq_cnt++;
        return 0;
    }
    
    ring->stats_enq_err_cnt++;

    return -1;
}

int dkfw_rcv_data_from_other_core_q(int core_seq, int core_q_num, void **data_burst, int max_data_num)
{
    DKFW_RING *ring = &g_other_core[core_seq].data_to_me_q[core_q_num];
    int nb_rx = rte_ring_sc_dequeue_burst(ring->dkfw_ring, data_burst, max_data_num, NULL);

    ring->stats_deq_cnt += nb_rx;

    return nb_rx;
}

// 从第core_seq个other核的每个（业务核数量）队列中收到的包数
void dkfw_pkt_rcv_from_other_core_stat(int core_seq, uint64_t *stats)
{
    int i;
    DKFW_RING *ring;

    for(i=0;i<g_pkt_process_core_num;i++){
        ring = &g_other_core[core_seq].data_to_me_q[i];
        stats[i] = ring->stats_deq_cnt;
    }
}

/* 控制通讯相关，后续使用 */
DKFW_IPC_MSG *dkfw_ipc_rcv_msg(void)
{
    void *msg;
    int ret;

    if(likely(rte_ring_empty(g_core_me->ipc_to_me))){
        return NULL;
    }

    ret = rte_ring_dequeue(g_core_me->ipc_to_me, &msg);

    return ret ? NULL : (DKFW_IPC_MSG *)msg;
}

/* 控制通讯相关，后续使用 */
int dkfw_ipc_send_response_msg(DKFW_IPC_MSG *msg)
{
    return rte_ring_enqueue(g_core_me->ipc_to_back, msg);
}

