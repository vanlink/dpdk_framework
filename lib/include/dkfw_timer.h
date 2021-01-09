#ifndef _DPDK_FRAMEWORK_TIMER_H
#define _DPDK_FRAMEWORK_TIMER_H

#include "dkfw_list.h"

struct timer_list;

typedef void (*linux_timer_cb_t)(struct timer_list *timer, unsigned long arg);

struct timer_list {
    struct list_head entry;
    unsigned long expires;
    linux_timer_cb_t function;
    unsigned long data;
};

#define TVN_BITS (6)
#define TVR_BITS (19)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

typedef struct tvec_s {
    struct list_head vec[TVN_SIZE];
} tvec_t;

typedef struct tvec_root_s {
    struct list_head vec[TVR_SIZE];
} tvec_root_t;

struct tvec_t_base_s {
    unsigned long timer_ticks;
    tvec_root_t tv1;
    tvec_t tv2;
    tvec_t tv3;
    tvec_t tv4;
    tvec_t tv5;
};
typedef struct tvec_t_base_s tvec_base_t;

extern int dkfw_run_timer(tvec_base_t *tvec_bases, unsigned long absms);
extern void dkfw_init_timers(tvec_base_t *tvec_bases, unsigned long absms);
extern int dkfw_start_timer(tvec_base_t *tvec_bases, struct timer_list *timer, linux_timer_cb_t cb, void *arg, unsigned long absms);
extern int dkfw_restart_timer(tvec_base_t *tvec_bases, struct timer_list *timer, unsigned long absms);
extern int dkfw_stop_timer(struct timer_list *timer);

#endif

