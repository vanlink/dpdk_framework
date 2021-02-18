#ifndef _DPDK_FRAMEWORK_PROFILE_H
#define _DPDK_FRAMEWORK_PROFILE_H
#include <stdint.h>
#include "cjson/cJSON.h"

#define MAX_PROFILE_ITEM_NUM 16
#define DKFW_PROFILE_LOOP_CNT 1

typedef struct _DKFW_PROFILE_TAG {

    uint64_t loops_cnt;

    uint64_t all_time;

    int item_cnt;
    uint64_t items_time[MAX_PROFILE_ITEM_NUM];
    uint64_t items_time_cnt[MAX_PROFILE_ITEM_NUM];

    int single_cnt;
    uint64_t single_time[MAX_PROFILE_ITEM_NUM];
    uint64_t single_time_cnt[MAX_PROFILE_ITEM_NUM];

    uint64_t tmp_start;
    uint64_t tmp_items_start[MAX_PROFILE_ITEM_NUM];
    uint64_t tmp_single_start[MAX_PROFILE_ITEM_NUM];
} DKFW_PROFILE;

extern void dkfw_profile_init(DKFW_PROFILE *profile, int item_cnt, int single_cnt);
extern cJSON *dkfw_profile_to_json(DKFW_PROFILE *profile);

#define DKFW_PROFILE_START(profile,t) do { (profile)->tmp_start = t; } while(0)
#if DKFW_PROFILE_LOOP_CNT
#define DKFW_PROFILE_END(profile,t) do { (profile)->loops_cnt++; (profile)->all_time += ((t) - (profile)->tmp_start); } while(0)
#else
#define DKFW_PROFILE_END(profile,t) do { (profile)->all_time += ((t) - (profile)->tmp_start); } while(0)
#endif

#define DKFW_PROFILE_ITEM_START(profile,t,i) do { profile->tmp_items_start[i] = t; } while(0)
#if DKFW_PROFILE_LOOP_CNT
#define  DKFW_PROFILE_ITEM_END(profile,t,i) do { profile->items_time[i] += ((t) - profile->tmp_items_start[i]); profile->items_time_cnt[i]++; } while(0)
#else
#define  DKFW_PROFILE_ITEM_END(profile,t,i) do { profile->items_time[i] += ((t) - profile->tmp_items_start[i]); } while(0)
#endif

#define DKFW_PROFILE_SINGLE_START(profile,t,i) do { profile->tmp_single_start[i] = t; } while(0)
#if DKFW_PROFILE_LOOP_CNT
#define  DKFW_PROFILE_SINGLE_END(profile,t,i) do { profile->single_time[i] += ((t) - profile->tmp_single_start[i]); profile->single_time_cnt[i]++; } while(0)
#else
#define  DKFW_PROFILE_SINGLE_END(profile,t,i) do { profile->single_time[i] += ((t) - profile->tmp_single_start[i]); } while(0)
#endif

#endif

