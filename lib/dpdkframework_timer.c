#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dkfw_timer.h"

#define timer_pending(timer) ((timer)->entry.next != NULL)

#define typecheck(type, x) \
        ({	type __dummy; \
        typeof(x) __dummy2; \
        (void)(&__dummy == &__dummy2); \
        1; \
        })

#define time_after_eq(a, b) \
        (typecheck(unsigned long, a) && \
        typecheck(unsigned long, b) && \
        ((long)(a) - (long)(b) >= 0))

static void internal_add_timer(tvec_base_t * base, struct timer_list * timer)
{
    unsigned long expires = timer->expires;
    unsigned long idx = expires - base->timer_ticks;

    struct list_head * vec;
    if(idx < TVR_SIZE) {
        int i = expires & TVR_MASK;

        vec = base->tv1.vec + i;
    }
    else if(idx < 1ULL << (TVR_BITS + TVN_BITS)) {
        int i = (expires >> TVR_BITS) &TVN_MASK;

        vec = base->tv2.vec + i;
    }
    else if(idx < 1ULL << (TVR_BITS + 2 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + TVN_BITS)) &TVN_MASK;

        vec = base->tv3.vec + i;
    }
    else if(idx < 1ULL << (TVR_BITS + 3 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) &TVN_MASK;

        vec = base->tv4.vec + i;
    }
    else if((signed long)
    idx < 0) {
        vec = base->tv1.vec + (base->timer_ticks & TVR_MASK);
    }
    else {
        int i;

        if(idx > 0xffffffffUL) {
            idx = 0xffffffffUL;
            expires = idx + base->timer_ticks;
        }

        i = (expires >> (TVR_BITS + 3 * TVN_BITS)) &TVN_MASK;
        vec = base->tv5.vec + i;
    }

    list_add_tail(&timer->entry, vec);
}

static void init_timer(struct timer_list * timer)
{
    timer->entry.next = NULL;
}

static inline void detach_timer(struct timer_list * timer, int clear_pending)
{
    struct list_head * entry = &timer->entry;
    __list_del(entry->prev, entry->next);
    if(clear_pending) {
        entry->next = NULL;
    }

    entry->prev = LIST_POISON2;
}

static int __mod_timer(tvec_base_t *tvec_bases, struct timer_list * timer, unsigned long expires)
{
    tvec_base_t * new_base;
    int ret = 0;

    if(timer_pending(timer)) {
        detach_timer(timer, 0);
        ret = 1;
    }

    new_base = tvec_bases;
    
    timer->expires = expires;
    internal_add_timer(new_base, timer);
    return ret;
}

static int del_timer(struct timer_list * timer)
{
    int ret = 0;

    if(timer_pending(timer)) {
        detach_timer(timer, 1);
        ret = 1;
    }

    return ret;
}

static int cascade(tvec_base_t * base, tvec_t * tv, int index)
{
    struct list_head * head, *curr;
    head = tv->vec + index;
    curr = head->next;
    while(curr != head) {
        struct timer_list * tmp;
        tmp = list_entry(curr, struct timer_list, entry);
        curr = curr->next;
        internal_add_timer(base, tmp);
    }

    INIT_LIST_HEAD(head);
    return index;
}

#define INDEX(N) (base->timer_ticks >> (TVR_BITS + N * TVN_BITS)) & TVN_MASK

static inline void __run_timers(tvec_base_t * base, unsigned long absms)
{
    struct timer_list * timer;
    while(time_after_eq(absms, base->timer_ticks)) {
        struct list_head work_list = LIST_HEAD_INIT(work_list);
        struct list_head * head = &work_list;
        int index = base->timer_ticks & TVR_MASK;

        if(!index && (!cascade(base, &base->tv2, INDEX(0))) && (!cascade(base, &base->tv3, INDEX(1))) &&
             !cascade(base, & base->tv4, INDEX(2))) {
            cascade(base, &base->tv5, INDEX(3));
        }

        ++base->timer_ticks;
        list_splice_init(base->tv1.vec + index, &work_list);
        while(!list_empty(head)) {
            void(*fn) (struct timer_list *, unsigned long);
            unsigned long data;

            timer = list_entry(head->next, struct timer_list, entry);
            fn = timer->function;
            data = timer->data;
            detach_timer(timer, 1);
            {
                fn(timer, data);
            }
        }
    }

}

int dkfw_run_timer(tvec_base_t *tvec_bases, unsigned long absms)
{
    tvec_base_t * base = tvec_bases;

    if(time_after_eq(absms, base->timer_ticks)) {
        __run_timers(base, absms);
        return 1;
    }

    return 0;
}

void dkfw_init_timers(tvec_base_t *tvec_bases, unsigned long absms)
{
    int j;
    tvec_base_t * base;

    memset(tvec_bases, 0, sizeof(tvec_base_t));

    base = tvec_bases;
    for(j = 0; j < TVN_SIZE; j++) {
        INIT_LIST_HEAD(base->tv5.vec + j);
        INIT_LIST_HEAD(base->tv4.vec + j);
        INIT_LIST_HEAD(base->tv3.vec + j);
        INIT_LIST_HEAD(base->tv2.vec + j);
    }

    for(j = 0; j < TVR_SIZE; j++) {
        INIT_LIST_HEAD(base->tv1.vec + j);
    }

    base->timer_ticks = absms;
}

// ============ wrapper ===========

int dkfw_start_timer(tvec_base_t *tvec_bases, struct timer_list *timer, linux_timer_cb_t cb, void *arg, unsigned long absms)
{
    init_timer(timer);

    timer->function = cb;
    timer->data = (unsigned long)arg;
    timer->expires = absms;

    __mod_timer(tvec_bases, timer, timer->expires);

    return 0;
}

int dkfw_restart_timer(tvec_base_t *tvec_bases, struct timer_list *timer, unsigned long absms)
{
    timer->expires = absms;

    __mod_timer(tvec_bases, timer, timer->expires);

    return 0;
}

int dkfw_stop_timer(struct timer_list *timer)
{
    del_timer(timer);

    return 0;
}

