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

static int process_core_init_one(DKFW_CORE *core, int to_me_q_num)
{
    int i;
    char buff[128];
    
    printf("Init process core ind %d: to_me_q_num %d.\n", core->core_ind, to_me_q_num);
    core->pkts_to_me_q_num = to_me_q_num;

    for(i=0;i<to_me_q_num;i++){
        snprintf(buff, sizeof(buff), "pctome%d-%d", core->core_ind, i);
        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            core->pkts_to_me_q[i].dkfw_ring = rte_ring_create(buff, 4096, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
        } else {
            core->pkts_to_me_q[i].dkfw_ring = rte_ring_lookup(buff);
        }
        if(!core->pkts_to_me_q[i].dkfw_ring){
            printf("process_core_init_one %s err.\n", buff);
            return -1;
        }
    }

    if(core_init_ipc_rings(core) < 0){
        return -1;
    }

    return 0;
}

static int dispatch_core_init_one(DKFW_CORE *core)
{
    if(core_init_ipc_rings(core) < 0){
        return -1;
    }
    
    return 0;
}

static int other_core_init_one(DKFW_CORE *core)
{
    if(core_init_ipc_rings(core) < 0){
        return -1;
    }
    
    return 0;
}

int cores_init(DKFW_CONFIG *config)
{
    int i;
    int pkt_to_me_q_num;
    CORE_CONFIG *core_config_curr;
    DKFW_CORE *core_curr;
    
    memset(g_pkt_process_core, 0, sizeof(g_pkt_process_core));
    memset(g_pkt_distribute_core, 0, sizeof(g_pkt_distribute_core));
    memset(g_other_core, 0, sizeof(g_other_core));

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

    if(g_pkt_distribute_core_num){
        pkt_to_me_q_num = g_pkt_distribute_core_num;
    }else{
        pkt_to_me_q_num = g_pkt_process_core_num;
    }

    for(i=0;i<g_pkt_process_core_num;i++){
        if(process_core_init_one(&g_pkt_process_core[i], pkt_to_me_q_num) < 0){
            return -1;
        }
    }

    for(i=0;i<g_pkt_distribute_core_num;i++){
        if(dispatch_core_init_one(&g_pkt_distribute_core[i]) < 0){
            return -1;
        }
    }

    for(i=0;i<g_other_core_num;i++){
        if(other_core_init_one(&g_other_core[i]) < 0){
            return -1;
        }
    }

    return 0;
}

int dkfw_start_loop_raw(void *loop_arg)
{
    if(!g_core_me->core_func_raw){
        return -1;
    }

    rte_eal_mp_remote_launch(g_core_me->core_func_raw, loop_arg, CALL_MASTER);
    rte_eal_mp_wait_lcore();

    return 0;
}

int dkfw_send_pkt_to_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf *mbuf)
{
    DKFW_RING *ring = &g_pkt_process_core[process_core_seq].pkts_to_me_q[core_q_num];

    if(rte_ring_sp_enqueue(ring->dkfw_ring, mbuf) < 0){
        ring->stats_enq_err_cnt++;
        return -1;
    }

    ring->stats_enq_cnt++;

    return 0;
}

int dkfw_rcv_pkt_from_process_core_q(int process_core_seq, int core_q_num, struct rte_mbuf **pkts_burst, int max_pkts_num)
{
    DKFW_RING *ring = &g_pkt_process_core[process_core_seq].pkts_to_me_q[core_q_num];
    int nb_rx = rte_ring_sc_dequeue_burst(ring->dkfw_ring, (void **)pkts_burst, max_pkts_num, NULL);

    ring->stats_deq_cnt += nb_rx;

    return nb_rx;
}

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

int dkfw_ipc_send_response_msg(DKFW_IPC_MSG *msg)
{
    return rte_ring_enqueue(g_core_me->ipc_to_back, msg);
}

