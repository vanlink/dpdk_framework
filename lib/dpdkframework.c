#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <rte_eal.h>

#include "dkfw_core.h"
#include "dkfw_intf.h"

static char dpdk_argv[64][64];
static char *argv_in[64];

static int eal_init(DKFW_CONFIG *config)
{
    int i, ret = 0;
    int argc = 0;
    int my_core_ind = -1;
    int my_core_cnt = 0;

    strcpy(dpdk_argv[argc], "dkfw");
    argc++;

    if(config->nuique_name[0]){
        sprintf(dpdk_argv[argc], "--file-prefix=%s", config->nuique_name);
        argc++;
    }

    sprintf(dpdk_argv[argc], "--proc-type=%s", config->process_type == PROCESS_TYPE_PRIMARY ? "primary" : "secondary");
    argc++;

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_process[i].core_enabled && config->cores_pkt_process[i].core_is_me){
            my_core_ind = config->cores_pkt_process[i].core_ind;
            my_core_cnt++;
            break;
        }
    }

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_dispatch[i].core_enabled && config->cores_pkt_dispatch[i].core_is_me){
            my_core_ind = config->cores_pkt_dispatch[i].core_ind;
            my_core_cnt++;
            break;
        }
    }

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_other[i].core_enabled && config->cores_other[i].core_is_me){
            my_core_ind = config->cores_other[i].core_ind;
            my_core_cnt++;
            break;
        }
    }

    if(my_core_cnt != 1 || my_core_ind < 0){
        printf("Invalid my core.\n");
        return -1;
    }

    strcpy(dpdk_argv[argc], "-l");
    argc++;

    sprintf(dpdk_argv[argc], "%d", my_core_ind);
    argc++;

    for(i=0;i<MAX_PCI_NUM;i++){
        if(config->pcis_config[i].pci_name[0]){
            sprintf(dpdk_argv[argc], "--pci-whitelist=%s", config->pcis_config[i].pci_name);
            argc++;
        }
    }

    printf("rte_eal_init: ");
    for(i=0;i<argc;i++){
        printf("%s ", dpdk_argv[i]);
        argv_in[i] = dpdk_argv[i];
    }
    printf("\n");

    ret = rte_eal_init(argc, argv_in);
    if (ret < 0) {
        printf("Error with EAL initialization\n");
        ret = -1;
    }

    return ret;
}

int dkfw_init(DKFW_CONFIG *config)
{
    int i;

    config->cores_pkt_process_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_process[i].core_enabled){
            config->cores_pkt_process_num++;
            config->cores_pkt_process[i].core_seq = i;
        }else{
            break;
        }
    }

    config->cores_pkt_dispatch_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_dispatch[i].core_enabled){
            config->cores_pkt_dispatch_num++;
            config->cores_pkt_dispatch[i].core_seq = i;
        }else{
            break;
        }
    }

    config->cores_other_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_other[i].core_enabled){
            config->cores_other_num++;
            config->cores_other[i].core_seq = i;
        }else{
            break;
        }
    }

    if(eal_init(config) < 0){
        goto err;
    }

    if(interfaces_init(config->cores_pkt_process_num, config->cores_pkt_dispatch_num) < 0){
        goto err;
    }

    if(cores_init(config) < 0){
        goto err;
    }

    return 0;

err:

    rte_eal_cleanup();
    return -1;
}

void dkfw_exit(void)
{
    rte_eal_cleanup();
}



