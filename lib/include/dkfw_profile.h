#ifndef _DPDK_FRAMEWORK_PROFILE_H
#define _DPDK_FRAMEWORK_PROFILE_H

#define MAX_PROFILE_ITEM_NUM 16

typedef struct _DKFW_PROFILE_TAG {
    int item_cnt;
    
    uint64_t loops_cnt;
    uint64_t abs_time;

    uint64_t all_time;
    uint64_t items_time[MAX_PROFILE_ITEM_NUM];

    uint64_t tmp_start;
    uint64_t tmp_items_start[MAX_PROFILE_ITEM_NUM];
} DKFW_PROFILE;

extern void dkfw_profile_reset(int item_cnt);
extern void dkfw_profile_start(uint64_t t);
extern void dkfw_profile_end(uint64_t t);
extern void dkfw_profile_item_start(uint64_t t, int i);
extern void dkfw_profile_item_end(uint64_t t, int i);
extern DKFW_PROFILE *dkfw_profile_get(void);
extern void dkfw_profile_snapshot(DKFW_PROFILE *old);
extern DKFW_PROFILE *dkfw_profile_calc(DKFW_PROFILE *snap);

#endif

