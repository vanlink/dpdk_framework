#ifndef _DPDK_FRAMEWORK_PROFILE_H
#define _DPDK_FRAMEWORK_PROFILE_H
#include <stdint.h>

#define MAX_PROFILE_ITEM_NUM 16

typedef struct _DKFW_PROFILE_TAG {
    int item_cnt;
    
    uint64_t loops_cnt;

    uint64_t all_time;
    uint64_t items_time[MAX_PROFILE_ITEM_NUM];

    uint64_t tmp_start;
    uint64_t tmp_items_start[MAX_PROFILE_ITEM_NUM];
} DKFW_PROFILE;

extern void dkfw_profile_init(DKFW_PROFILE *profile, int item_cnt);

#define DKFW_PROFILE_START(profile,t) do { (profile)->tmp_start = t } while(0)
#define DKFW_PROFILE_END(profile,t) do { \
    (profile)->loops_cnt++; \
    (profile)->all_time += ((t) - (profile)->tmp_start); } while(0)

#define DKFW_PROFILE_ITEM_START(profile,t,i) do { profile->tmp_items_start[i] = t; } while(0)
#define  DKFW_PROFILE_ITEM_END(profile,t,i) do { profile->items_time[i] += ((t) - profile->tmp_items_start[i]); } while(0)

#endif

