#ifndef _DPDK_FRAMEWORK_MEMORY_H
#define _DPDK_FRAMEWORK_MEMORY_H

#define DKFW_GLOBAL_SHARED_MEM_SIZE (16 * 1024 *1024)

extern int global_init_sharemem(void);
extern void *dkfw_global_sharemem_get(void);

#endif

