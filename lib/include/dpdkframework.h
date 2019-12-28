#ifndef _DPDK_FRAMEWORK_H
#define _DPDK_FRAMEWORK_H

#include "dkfw_config.h"

extern int dkfw_init(DKFW_CONFIG *config);
extern void dkfw_exit(void);
extern int dkfw_start_loop_raw(void *loop_arg);
#endif

