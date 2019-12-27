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

static int g_pkt_process_core_num = 0;
static DKFW_CORE g_pkt_process_core[MAX_CORES_PER_ROLE];

static int g_pkt_distribute_core_num = 0;
static DKFW_CORE g_pkt_distribute_core[MAX_CORES_PER_ROLE];

static int g_other_core_num = 0;
static DKFW_CORE g_other_core[MAX_CORES_PER_ROLE];

static int gkfw_process_core_init_one(DKFW_CORE *core, int to_me_q_num)
{
    int i;
    char buff[128];
    
    printf("Init process core ind %d: to_me_q_num %d.\n", core->core_ind, to_me_q_num);
    core->pkts_to_me_q_num = to_me_q_num;

    for(i=0;i<to_me_q_num;i++){
        snprintf(buff, sizeof(buff), "pctome%d-%d", core->core_ind, i);
        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            core->pkts_to_me_q[i] = rte_ring_create(buff, 2048, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
        } else {
            core->pkts_to_me_q[i] = rte_ring_lookup(buff);
        }
        if(!core->pkts_to_me_q[i]){
            printf("gkfw_process_core_init_one %s err.\n", buff);
            return -1;
        }
    }
    
    return 0;
}

int gkfw_cores_init(DKFW_CONFIG *config)
{
    int i;
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
    }
    printf("We have other cores: %d\n", g_other_core_num);

    for(i=0;i<g_pkt_process_core_num;i++){
        if(gkfw_process_core_init_one(&g_pkt_process_core[i], g_pkt_distribute_core_num) < 0){
            return -1;
        }
    }

    return 0;
}


