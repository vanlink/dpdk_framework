#ifndef _DPDK_FRAMEWORK_MEMPOOL_H
#define _DPDK_FRAMEWORK_MEMPOOL_H

typedef struct _DKFW_MEMHEADER_t DKFW_MEMHEADER;

typedef struct _DKFW_MEMPOOL_t {
    DKFW_MEMHEADER *first_header;
    char *pool_mem;
} DKFW_MEMPOOL;

typedef struct _DKFW_MEMHEADER_t {
    struct _DKFW_MEMHEADER_t *next;
    DKFW_MEMPOOL *pool;
} DKFW_MEMHEADER;

extern DKFW_MEMPOOL *dkfw_mempool_create(int size, int cnt);
extern void *dkfw_mempool_alloc(DKFW_MEMPOOL *pool);
extern void dkfw_mempool_free(void *ptr);

#endif

