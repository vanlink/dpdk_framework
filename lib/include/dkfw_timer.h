#ifndef _DPDK_FRAMEWORK_TIMER_H
#define _DPDK_FRAMEWORK_TIMER_H

#include "dkfw_list.h"

struct timer_base_s;
struct timer_list;

typedef void (*linux_timer_cb_t)(struct timer_list *timer, unsigned long arg);

struct timer_list {
    struct list_head entry;
    unsigned long expires;
    linux_timer_cb_t function;
    unsigned long data;
    struct timer_base_s * base;
};

extern int dkfw_run_timer(unsigned long absms);
extern void dkfw_init_timers(unsigned long absms);
extern int dkfw_start_timer(struct timer_list *timer, linux_timer_cb_t cb, void *arg, unsigned long absms);
extern int dkfw_restart_timer(struct timer_list *timer, unsigned long absms);
extern int dkfw_stop_timer(struct timer_list *timer);

#endif

