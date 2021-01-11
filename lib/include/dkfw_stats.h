#ifndef _DPDK_FRAMEWORK_STATS_H
#define _DPDK_FRAMEWORK_STATS_H
#include "dkfw_config.h"

#define DKFW_STATS_KEYNAME_LEN 32

#define DKFW_STATS_TYPE_NONE          0
#define DKFW_STATS_TYPE_NUM           1
#define DKFW_STATS_TYPE_RESOURCE_POOL 2

typedef struct DKFW_ST_RESOURCE_POOL_TAG {
    unsigned long alloc_succ;
    unsigned long alloc_fail;
    unsigned long free;
} DKFW_ST_RESOURCE_POOL;

typedef struct DKFW_ST_CORE_TAG {
    union {
        unsigned long count;
        DKFW_ST_RESOURCE_POOL resource_pool;
    };
} DKFW_ST_CORE;

typedef struct DKFW_ST_ITEM_TAG {
    int type;
    char keyname[DKFW_STATS_KEYNAME_LEN];
    DKFW_ST_CORE stat_cores[MAX_CORES_PER_ROLE];
} DKFW_ST_ITEM;

typedef struct DKFW_STATS_TAG {
    int core_cnt;
    int max_id;

    DKFW_ST_ITEM *stat_items;
} DKFW_STATS;

extern DKFW_STATS *dkfw_stats_create(int core_cnt, int max_id);
extern int dkfw_stats_add_item(DKFW_STATS *stat, int id, int type, const char *keyname);
extern void dkfw_stats_cnt_incr(DKFW_STATS *stat, int id, int core);
extern void dkfw_stats_resource_pool_alloc_succ_incr(DKFW_STATS *stat, int id, int core);
extern void dkfw_stats_resource_pool_alloc_fail_incr(DKFW_STATS *stat, int id, int core);
extern void dkfw_stats_resource_pool_alloc_free_incr(DKFW_STATS *stat, int id, int core);

#endif

