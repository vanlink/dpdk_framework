#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

#include <stdio.h>

#define dkfw_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define dkfw_container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - dkfw_offsetof(type,member) );})

#define LIST_POISON2 ((void *) 0x00200200)

struct list_head {
    struct list_head * next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
        } while (0)

static inline void __list_add(struct list_head * new, 
    struct list_head * prev, 
    struct list_head * next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_tail(struct list_head * new, struct list_head * head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline int list_empty(const struct list_head * head)
{
    return head->next == head;
}

static inline void __list_splice(struct list_head * list, 
    struct list_head * head)
{
    struct list_head * first = list->next;
    struct list_head * last = list->prev;
    struct list_head * at = head->next;
    first->prev = head;
    head->next = first;
    last->next = at;
    at->prev = last;
}

static inline void list_splice_init(struct list_head * list, 
    struct list_head * head)
{
    if(!list_empty(list)) {
        __list_splice(list, head);
        INIT_LIST_HEAD(list);
    }
}

#define list_entry(ptr, type, member) \
        dkfw_container_of(ptr, type, member)

#endif

