#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#include "dkfw_memory.h"

static const struct rte_memzone *global_shared_mem_rte = NULL;
static void *global_shared_mem = NULL;

static void get_sharemem_name(char *buff)
{
    sprintf(buff, "dkfwsharemem");
}

int global_init_sharemem(void)
{
    char buff[128];

    get_sharemem_name(buff);

    if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
        printf("Create ");
        global_shared_mem_rte = rte_memzone_reserve_aligned(buff, 1*1024*1024, SOCKET_ID_ANY, 0, 64);
    }else{
        printf("Lookup ");
        global_shared_mem_rte = rte_memzone_lookup(buff);
    }

    printf("global shared mem [%s] ... ", buff);
    
    if(!global_shared_mem_rte){
        printf("fail.\n");
        return -1;
    }
    printf("ok.\n");
    global_shared_mem = global_shared_mem_rte->addr;

    return 0;
}

void *dkfw_global_sharemem_get(void)
{
    char buff[128];
    const struct rte_memzone *m;

    get_sharemem_name(buff);

    m = rte_memzone_lookup(buff);

    return m ? m->addr : NULL;
}
