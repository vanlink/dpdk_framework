#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_malloc.h>

#include "dkfw_stats.h"

DKFW_STATS *dkfw_stats_create(int core_cnt, int max_id)
{
    DKFW_STATS *stats = NULL;

    stats = rte_zmalloc(NULL, sizeof(DKFW_STATS) + sizeof(DKFW_ST_ITEM) * max_id, RTE_CACHE_LINE_SIZE);
    if(!stats){
        return NULL;
    }

    stats->core_cnt = core_cnt;
    stats->max_id = max_id;

    return stats;
}

int dkfw_stats_create_with_address(DKFW_STATS *stats, int core_cnt, int max_id)
{
    int size = sizeof(DKFW_STATS) + sizeof(DKFW_ST_ITEM) * max_id;

    stats->core_cnt = core_cnt;
    stats->max_id = max_id;

    memset(stats, 0, size);

    return size;
}

int dkfw_stats_add_item(DKFW_STATS *stat, int id, int type, const char *keyname)
{
    if(id >= stat->max_id){
        return -1;
    }

    if(type == DKFW_STATS_TYPE_NONE){
        return -1;
    }

    if(stat->stat_items[id].type != DKFW_STATS_TYPE_NONE){
        return -1;
    }

    strncpy(stat->stat_items[id].keyname, keyname, DKFW_STATS_KEYNAME_LEN - 1);
    stat->stat_items[id].type = type;

    return 0;
}

int dkfw_stats_config_dup(DKFW_STATS *stat, DKFW_STATS *stat_dst)
{
    int id;
    DKFW_ST_ITEM *item, *item_dst;

    stat_dst->core_cnt = stat->core_cnt;
    stat_dst->max_id = stat->max_id;

    for(id=0;id<stat->max_id;id++){
        item = &stat->stat_items[id];
        item_dst = &stat_dst->stat_items[id];

        item_dst->type = item->type;
        strcpy(item_dst->keyname, item->keyname);
    }

    return 0;
}

int dkfw_stats_cores_sum(DKFW_STATS *stat, DKFW_STATS *stat_sum)
{
    int id, core;
    DKFW_ST_ITEM *item, *item_sum;

    stat_sum->core_cnt = stat->core_cnt;
    stat_sum->max_id = stat->max_id;

    for(id=0;id<stat->max_id;id++){
        item = &stat->stat_items[id];
        item_sum = &stat_sum->stat_items[id];
        memset(&item_sum->stat_cores[0], 0, sizeof(DKFW_ST_CORE));
        for(core=0;core<stat->core_cnt;core++){
            if(item->type == DKFW_STATS_TYPE_NUM){
                item_sum->stat_cores[0].count += item->stat_cores[core].count;
            }else if(item->type == DKFW_STATS_TYPE_RESOURCE_POOL){
                item_sum->stat_cores[0].resource_pool.alloc_succ += item->stat_cores[core].resource_pool.alloc_succ;
                item_sum->stat_cores[0].resource_pool.alloc_fail += item->stat_cores[core].resource_pool.alloc_fail;
                item_sum->stat_cores[0].resource_pool.free += item->stat_cores[core].resource_pool.free;
            }
        }
    }

    return 0;
}

