#ifndef _DPDK_FRAMEWORK_STATS_H
#define _DPDK_FRAMEWORK_STATS_H
#include "dkfw_config.h"
#include "cjson/cJSON.h"

#define DKFW_STATS_CONF_PEAK 0

#define DKFW_STATS_KEYNAME_LEN 32

#define DKFW_STATS_TYPE_NONE          0
#define DKFW_STATS_TYPE_NUM           1
#define DKFW_STATS_TYPE_RESOURCE_POOL 2
#define DKFW_STATS_TYPE_PAIR          3

typedef struct DKFW_ST_RESOURCE_POOL_TAG {
    unsigned long alloc_succ;
    unsigned long alloc_fail;
    unsigned long free;

    unsigned long peak;

    unsigned long want_succ;
    unsigned long want_fail;
} DKFW_ST_RESOURCE_POOL;

typedef struct DKFW_ST_PAIR_TAG {
    unsigned long start;
    unsigned long stop;

    unsigned long peak;
} DKFW_ST_PAIR;

typedef struct DKFW_ST_CORE_TAG {
    union {
        unsigned long count;
        DKFW_ST_RESOURCE_POOL resource_pool;
        DKFW_ST_PAIR pair;
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

    DKFW_ST_ITEM stat_items[0];
} DKFW_STATS;

extern DKFW_STATS *dkfw_stats_create(int core_cnt, int max_id);
extern int dkfw_stats_create_with_address(DKFW_STATS *stats, int core_cnt, int max_id);
extern int dkfw_stats_add_item(DKFW_STATS *stat, int id, int type, const char *keyname);
extern int dkfw_stats_config_dup(DKFW_STATS *stat, DKFW_STATS *stat_dst);
extern int dkfw_stats_cores_sum(DKFW_STATS *stat, DKFW_STATS *stat_sum);
cJSON *dkfw_stats_to_json(DKFW_STATS *stat);

#define DKFW_STATS_CNT_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].count++
#if DKFW_STATS_CONF_PEAK
#define DKFW_STATS_PAIR_START_INCR(stat,id,core) do {      \
    unsigned long __tmp;                                   \
    (stat)->stat_items[id].stat_cores[core].pair.start++;  \
    __tmp = (stat)->stat_items[id].stat_cores[core].pair.start - (stat)->stat_items[id].stat_cores[core].pair.stop; \
    if(__tmp > (stat)->stat_items[id].stat_cores[core].pair.peak){ (stat)->stat_items[id].stat_cores[core].pair.peak = __tmp; } \
} while(0)
#else
#define DKFW_STATS_PAIR_START_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].pair.start++
#endif
#define DKFW_STATS_PAIR_STOP_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].pair.stop++

#if DKFW_STATS_CONF_PEAK
#define DKFW_STATS_RESOURCE_POOL_ALLOC_SUCC_INCR(stat,id,core) do {      \
    unsigned long __tmp;                                                 \
    (stat)->stat_items[id].stat_cores[core].resource_pool.alloc_succ++;  \
    __tmp = (stat)->stat_items[id].stat_cores[core].resource_pool.alloc_succ - (stat)->stat_items[id].stat_cores[core].resource_pool.free; \
    if(__tmp > (stat)->stat_items[id].stat_cores[core].resource_pool.peak){ (stat)->stat_items[id].stat_cores[core].resource_pool.peak = __tmp; } \
} while(0)
#else
#define DKFW_STATS_RESOURCE_POOL_ALLOC_SUCC_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].resource_pool.alloc_succ++
#endif

#define DKFW_STATS_RESOURCE_POOL_ALLOC_FAIL_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].resource_pool.alloc_fail++
#define DKFW_STATS_RESOURCE_POOL_ALLOC_FREE_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].resource_pool.free++
#define DKFW_STATS_RESOURCE_POOL_WANT_SUCCE_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].resource_pool.want_succ++
#define DKFW_STATS_RESOURCE_POOL_WANT_FAIL_INCR(stat,id,core) (stat)->stat_items[id].stat_cores[core].resource_pool.want_fail++

#endif

