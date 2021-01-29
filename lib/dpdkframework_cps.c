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

static inline uint64_t get_curr_cps(DKFW_CPS *dkfwcps, uint64_t ms)
{
    DKFW_CPS_SEG *seg = &dkfwcps->cps_segs[dkfwcps->cps_seg_curr_ind];
    uint64_t offset_ms;

    if(unlikely(!seg->seg_total_ms)){
        return seg->cps_start;
    }

    if(unlikely(!seg->seg_start_ms)){
        seg->seg_start_ms = ms;
        return seg->cps_start;
    }

    offset_ms = ms - seg->seg_start_ms;

    if(unlikely(offset_ms >= seg->seg_total_ms)){
        dkfwcps->cps_seg_curr_ind++;
        return seg->cps_end;
    }

    if(seg->cps_start == seg->cps_end){
        return seg->cps_start;
    }

    if(seg->cps_start < seg->cps_end){
        return seg->cps_start + offset_ms * (seg->cps_end - seg->cps_start) / seg->seg_total_ms;
    }else{
        return seg->cps_start - offset_ms * (seg->cps_start - seg->cps_end) / seg->seg_total_ms;
    }
}

uint64_t dkfw_cps_get(DKFW_CPS *dkfwcps, uint64_t tsc, uint64_t ms)
{
    uint64_t diff, cnt;
    uint64_t cps;

    if(unlikely(!dkfwcps->tsc_last)){
        dkfwcps->tsc_last = tsc;
        return 0;
    }

    cps = get_curr_cps(dkfwcps, ms);

    diff = (tsc - dkfwcps->tsc_last) * cps;

    if(likely(diff < dkfwcps->tsc_per_sec)){
        return 0;
    }

    cnt = diff / dkfwcps->tsc_per_sec;

    dkfwcps->tsc_last += dkfwcps->tsc_per_sec * cnt / cps;

    return cnt;
}

uint64_t dkfw_cps_limited_get(DKFW_CPS *dkfwcps, uint64_t tsc, uint64_t ms)
{
    uint64_t cnt = dkfw_cps_get(dkfwcps, tsc, ms);

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

