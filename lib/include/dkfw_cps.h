#ifndef _DPDK_FRAMEWORK_CPS_H
#define _DPDK_FRAMEWORK_CPS_H
#include <stdint.h>

#define DKFW_CPS_SEGS_MAX 8

typedef struct DKFW_CPS_SEG_TAG {
    uint64_t cps_start;
    uint64_t cps_end;

    uint64_t seg_total_ms;

    uint64_t seg_start_ms;
} DKFW_CPS_SEG;

typedef struct DKFW_CPS_TAG {
    uint64_t cps;
    uint64_t tsc_last;
    uint64_t tsc_per_sec;

    uint64_t send_cnt_remain;

    int cps_seg_curr_ind;
    DKFW_CPS_SEG cps_segs[DKFW_CPS_SEGS_MAX];
} DKFW_CPS;

extern void dkfw_cps_create(DKFW_CPS *dkfwcps, uint64_t cps, uint64_t tsc_per_sec);
extern uint64_t dkfw_cps_get(DKFW_CPS *dkfwcps, uint64_t tsc, uint64_t ms);
extern uint64_t dkfw_cps_limited_get(DKFW_CPS *dkfwcps, uint64_t tsc, uint64_t ms);

#endif

