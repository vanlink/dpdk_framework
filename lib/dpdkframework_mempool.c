#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_common.h>
#include <rte_mempool.h>

#include "dkfw_mempool.h"

#define HEADER_SIZE (RTE_CACHE_LINE_SIZE * 8)

static int poolcnt = 0;

DKFW_MEMPOOL *dkfw_mempool_create(int size, int cnt)
{
    int i;
    char buff[128];
    DKFW_MEMPOOL *pool = calloc(1, sizeof(DKFW_MEMPOOL));
    void *obj;
    DKFW_MEMHEADER *header;

    sprintf(buff, "dkfwmempool%d", poolcnt++);

    pool->dpdk_mempool = rte_mempool_create(buff, cnt + 64, size + HEADER_SIZE, 0, 0, NULL, NULL, NULL, NULL, SOCKET_ID_ANY, MEMPOOL_F_NO_IOVA_CONTIG);

    if(!pool->dpdk_mempool){
        printf("dkfw create mem pool %s error.\n", buff);
        return NULL;
    }

    for(i=0;i<cnt;i++){
        if(rte_mempool_get(pool->dpdk_mempool, (void **)&obj) < 0) {
            printf("dkfw create mem pool %s error obj.\n", buff);
            return NULL;
        }
        bzero(obj, size + HEADER_SIZE);
        header = (DKFW_MEMHEADER *)obj;
        header->pool = pool;
        header->next = pool->list;
        pool->list = header;
    }

    return pool;
}

void *dkfw_mempool_alloc(DKFW_MEMPOOL *pool)
{
    DKFW_MEMHEADER *header;

    if(unlikely(!pool->list)){
        return NULL;
    }

    header = pool->list;
    pool->list = pool->list->next;
    header->next = NULL;

    return (char *)header + HEADER_SIZE;
}

void dkfw_mempool_free(void *ptr)
{
    DKFW_MEMHEADER *header = (DKFW_MEMHEADER *)((char *)ptr - HEADER_SIZE);

    header->next = header->pool->list;
    header->pool->list = header;
}

