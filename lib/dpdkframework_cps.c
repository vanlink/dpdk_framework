#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_branch_prediction.h>

#include "dkfw_cps.h"

void dkfw_cps_create(DKFW_CPS *dkfwcps, uint64_t cps, uint64_t tsc_per_sec)
{
    memset(dkfwcps, 0, sizeof(DKFW_CPS));

    dkfwcps->cps = cps;
    dkfwcps->tsc_per_sec = tsc_per_sec;
}

uint64_t dkfw_cps_get(DKFW_CPS *dkfwcps, uint64_t tsc)
{
    uint64_t diff, cnt;

    if(unlikely(!dkfwcps->tsc_last)){
        dkfwcps->tsc_last = tsc;
        return 0;
    }

    diff = (tsc - dkfwcps->tsc_last) * dkfwcps->cps;

    if(likely(diff < dkfwcps->tsc_per_sec)){
        return 0;
    }

    cnt = diff / dkfwcps->tsc_per_sec;

    dkfwcps->tsc_last += dkfwcps->tsc_per_sec * cnt / dkfwcps->cps;

    return cnt;
}

uint64_t dkfw_cps_limited_get(DKFW_CPS *dkfwcps, uint64_t tsc)
{
    uint64_t cnt = dkfw_cps_get(dkfwcps, tsc);

    if(likely(cnt == 0)){
        if(unlikely(dkfwcps->send_cnt_remain)){
            dkfwcps->send_cnt_remain--;
            return 1;
        }
        return 0;
    }

    if(likely(cnt == 1)){
        return 1;
    }

    dkfwcps->send_cnt_remain += (cnt - 1);

    if(dkfwcps->send_cnt_remain > 64){
        dkfwcps->send_cnt_remain = 64;
    }

    return 1;
}

