#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_common.h>

#include "dkfw_profile.h"

static DKFW_PROFILE g_profile;

void dkfw_profile_reset(int item_cnt)
{
    memset(&g_profile, 0, sizeof(g_profile));
    g_profile.item_cnt = (item_cnt ? item_cnt : MAX_PROFILE_ITEM_NUM);
}

void dkfw_profile_start(uint64_t t)
{
    g_profile.tmp_start = t;
}

void dkfw_profile_end(uint64_t t)
{
    g_profile.loops_cnt++;
    g_profile.all_time += (t - g_profile.tmp_start);
}

void dkfw_profile_item_start(uint64_t t, int i)
{
    g_profile.tmp_items_start[i] = t;
}

void dkfw_profile_item_end(uint64_t t, int i)
{
    g_profile.abs_time = t;
    g_profile.items_time[i] += (t - g_profile.tmp_items_start[i]);
}

DKFW_PROFILE *dkfw_profile_get(void)
{
    return &g_profile;
}

void dkfw_profile_result_get(DKFW_PROFILE *dst)
{
    int i;
        
    dst->item_cnt = g_profile.item_cnt;
    dst->loops_cnt = g_profile.loops_cnt;
    dst->all_time = g_profile.all_time;

    for(i=0;i<g_profile.item_cnt;i++){
        dst->items_time[i] = g_profile.items_time[i];
    }
}

void dkfw_profile_snapshot(DKFW_PROFILE *old)
{
    memcpy(old, &g_profile, sizeof(g_profile));
}

DKFW_PROFILE *dkfw_profile_calc(DKFW_PROFILE *snap)
{
    int i;
    
    snap->loops_cnt = g_profile.loops_cnt - snap->loops_cnt;
    snap->abs_time = g_profile.abs_time - snap->abs_time;
    
    snap->all_time = g_profile.all_time - snap->all_time;
    for(i=0;i<MAX_PROFILE_ITEM_NUM;i++){
        snap->items_time[i] = g_profile.items_time[i] - snap->items_time[i];
    }

    if(snap->all_time){
        for(i=0;i<MAX_PROFILE_ITEM_NUM;i++){
            snap->items_time[i] = snap->items_time[i] * 100 / snap->all_time;
        }
    }else{
        for(i=0;i<MAX_PROFILE_ITEM_NUM;i++){
            snap->items_time[i] = 100;
        }
    }

    return snap;
}

