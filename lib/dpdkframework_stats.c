#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_malloc.h>

#include "dkfw_stats.h"

DKFW_STATS *dkfw_stats_create(int core_cnt, int max_id)
{
    DKFW_STATS *stats = NULL;

    stats = rte_zmalloc(NULL, sizeof(DKFW_STATS), RTE_CACHE_LINE_SIZE);
    if(!stats){
        return NULL;
    }

    stats->stat_items = rte_zmalloc(NULL, sizeof(DKFW_ST_ITEM) * max_id, RTE_CACHE_LINE_SIZE);
    if(!stats->stat_items){
        return NULL;
    }

    stats->core_cnt = core_cnt;
    stats->max_id = max_id;

    return stats;
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

