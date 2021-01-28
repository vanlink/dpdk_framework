#ifndef _DPDK_FRAMEWORK_CPS_H
#define _DPDK_FRAMEWORK_CPS_H
#include <stdint.h>

typedef struct DKFW_CPS_TAG {
    uint64_t cps;
    uint64_t tsc_last;
    uint64_t tsc_per_sec;

    uint64_t send_cnt_remain;

} DKFW_CPS;

extern void dkfw_cps_create(DKFW_CPS *dkfwcps, uint64_t cps, uint64_t tsc_per_sec);
extern uint64_t dkfw_cps_get(DKFW_CPS *dkfwcps, uint64_t tsc);
extern uint64_t dkfw_cps_limited_get(DKFW_CPS *dkfwcps, uint64_t tsc);

#endif

