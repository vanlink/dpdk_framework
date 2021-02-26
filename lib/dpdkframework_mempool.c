#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dkfw_mempool.h"

#define HEADER_SIZE sizeof(DKFW_MEMHEADER)

DKFW_MEMPOOL *dkfw_mempool_create(int size, int cnt)
{
    int i;
    DKFW_MEMPOOL *pool = calloc(1, sizeof(DKFW_MEMPOOL));
    DKFW_MEMHEADER *header;
    int elesize = HEADER_SIZE + size;

    if(!pool){
        return NULL;
    }

    pool->pool_mem = calloc(1, elesize * cnt);
    if(!pool->pool_mem){
        free(pool);
        return NULL;
    }

    for(i=0;i<cnt;i++){
        header = (DKFW_MEMHEADER *)(pool->pool_mem + i * elesize);
        header->pool = pool;
        header->next = pool->first_header;
        pool->first_header = header;
    }

    return pool;
}

void *dkfw_mempool_alloc(DKFW_MEMPOOL *pool)
{
    DKFW_MEMHEADER *header;

    if(pool->first_header){
        header = pool->first_header;
        pool->first_header = pool->first_header->next;
        header->next = NULL;
    }else{
        return NULL;
    }

    return (char *)header + HEADER_SIZE;
}

void dkfw_mempool_free(void *ptr)
{
    DKFW_MEMHEADER *header = (DKFW_MEMHEADER *)((char *)ptr - HEADER_SIZE);

    header->next = header->pool->first_header;
    header->pool->first_header = header;
}

