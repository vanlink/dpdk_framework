#ifndef _DPDK_FRAMEWORK_CONFIG_H
#define _DPDK_FRAMEWORK_CONFIG_H

#include <rte_launch.h>

enum {
    CORE_ROLE_PKT_PROCESS,
    CORE_ROLE_PKT_DISPATCH,
    CORE_ROLE_OTHER,
};

enum {
    PROCESS_TYPE_PRIMARY,
    PROCESS_TYPE_SECONDARY,
};

typedef struct _CORE_CONFIG_TAG {
    int core_enabled;
    int core_seq;
    int core_ind;
    int core_role;
    int core_is_me;
    
    lcore_function_t *core_func_raw;
} CORE_CONFIG;

typedef struct _PCI_CONFIG_TAG {
    char pci_name[64];
} PCI_CONFIG;

#define MAX_CORES_PER_ROLE 32
#define MAX_PCI_NUM 8

typedef struct _DKFW_CONFIG_TAG {
    char nuique_name[64];

    int process_type;

    int cores_pkt_process_num;
    CORE_CONFIG cores_pkt_process[MAX_CORES_PER_ROLE];

    int cores_pkt_dispatch_num;
    CORE_CONFIG cores_pkt_dispatch[MAX_CORES_PER_ROLE];

    int cores_other_num;
    CORE_CONFIG cores_other[MAX_CORES_PER_ROLE];

    PCI_CONFIG pcis_config[MAX_PCI_NUM];

} DKFW_CONFIG;

#endif

