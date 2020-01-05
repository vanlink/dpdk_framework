#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_common.h>
#include <rte_ring.h>
#include <rte_eal.h>
#include <rte_mempool.h>

#include "dkfw_config.h"
#include "dkfw_ipc.h"

#define IPC_MEMPOOL_NAME "ipcmsgpool"
#define IPC_MSG_LENGTH_MAX   (16 * 1024)

static char dpdk_argv[64][64];
static char *argv_in[64];

static struct rte_mempool *ipc_message_pool;

int dkfw_ipc_client_init(char *nuique_name)
{
    int i, ret;
    int argc = 0;

    strcpy(dpdk_argv[argc], "dkfw");
    argc++;

    if(nuique_name && nuique_name[0]){
        sprintf(dpdk_argv[argc], "--file-prefix=%s", nuique_name);
        argc++;
    }

    sprintf(dpdk_argv[argc], "--proc-type=secondary");
    argc++;

/*
    strcpy(dpdk_argv[argc], "-l");
    argc++;

    sprintf(dpdk_argv[argc], "0");
    argc++;
    */

    for(i=0;i<argc;i++){
        argv_in[i] = dpdk_argv[i];
    }

    ret = rte_eal_init(argc, argv_in);
    if (ret < 0) {
        rte_eal_cleanup();
        printf("Error with EAL initialization\n");
        return -1;
    }

    ipc_message_pool = rte_mempool_lookup(IPC_MEMPOOL_NAME);

    if (!ipc_message_pool) {
        rte_eal_cleanup();
        printf("Find ipc msg mempool failed.\n");
        return -1;
    }

    return 0;
}

void dkfw_ipc_client_exit(void)
{
    rte_eal_cleanup();
}

static void get_ipc_ring_name(int core_role, int core_seq, int ring_type, char *ringname)
{
    char ringtypename[16];
    char corerolename[16];

    if(ring_type == IPC_TO_CORE_RING){
        strcpy(ringtypename, "tc");
    }else{
        strcpy(ringtypename, "tbk");
    }

    if(core_role == CORE_ROLE_PKT_PROCESS){
        strcpy(corerolename, "pkt");
    }else if(core_role == CORE_ROLE_PKT_DISPATCH){
        strcpy(corerolename, "dpt");
    }else{
        strcpy(corerolename, "otr");
    }

    sprintf(ringname, "%s%s-%d", corerolename, ringtypename, core_seq);
}

struct rte_ring *ipc_create_or_lookup_ring(int core_role, int core_seq, int ring_type)
{
    struct rte_ring *ring;
    char name[64];

    get_ipc_ring_name(core_role, core_seq, ring_type, name);

    if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
        ring = rte_ring_create(name, 1, SOCKET_ID_ANY, 0);
    } else {
        ring = rte_ring_lookup(name);
    }

    return ring;
}

int init_ipc_msg_pool(int num)
{
    struct rte_mempool *tgipc_message_pool;

    if (rte_eal_process_type() != RTE_PROC_PRIMARY){
        return 0;
    }

    tgipc_message_pool = rte_mempool_create(IPC_MEMPOOL_NAME,
       num,
       IPC_MSG_LENGTH_MAX, 0, 0,
       NULL, NULL, NULL, NULL,
       SOCKET_ID_ANY, 0);
    
    if (!tgipc_message_pool) {
        printf("Create ipc msg mempool failed.\n");
        return -1;
    }

    return 0;
}

DKFW_IPC_MSG *dkfw_ipc_msg_alloc(void)
{
    void *msg;
    
    if (rte_mempool_get(ipc_message_pool, &msg) < 0) {
        return NULL;
    }

    memset(msg, 0, sizeof(DKFW_IPC_MSG));

    return (DKFW_IPC_MSG *)msg;
}

void dkfw_ipc_msg_free(DKFW_IPC_MSG *msg)
{
    rte_mempool_put(ipc_message_pool, msg);
}

int dkfw_ipc_client_send_msg_to_core(int core_role, int core_seq, DKFW_IPC_MSG *msg)
{
    struct rte_ring *ring = ipc_create_or_lookup_ring(core_role, core_seq, IPC_TO_CORE_RING);
    int ret;

    if(!ring){
        printf("Failed to find core ring.\n");
        return -1;
    }

    ret = rte_ring_enqueue(ring, (void *)msg);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

DKFW_IPC_MSG *dkfw_ipc_client_rcv_msg_from_core(int core_role, int core_seq, int timeout)
{
    int i, ret;
    struct rte_ring *ring = ipc_create_or_lookup_ring(core_role, core_seq, IPC_TO_BACK_RING);
    void *obj;
    
    if(!ring){
        printf("Failed to find core ring.\n");
        return NULL;
    }

    if(timeout <= 0){
        timeout = 5;
    }

    for (i = 0; i < timeout * 1000; i++) {
        ret = rte_ring_dequeue(ring, &obj);
        if (!ret) {
            return (DKFW_IPC_MSG *)obj;
        }

        usleep(1000);
    }

    return NULL;
}

