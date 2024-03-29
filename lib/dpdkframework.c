#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <rte_eal.h>

#include "dkfw_core.h"
#include "dkfw_intf.h"
#include "dkfw_memory.h"

static char dpdk_argv[150][150];
static char *argv_in[150];

extern int init_ipc_msg_pool(int num);

/* 
初始化dpdk本体
返回0成功，其他失败
*/
static int eal_init(DKFW_CONFIG *config)
{
    int i, ret = 0;
    int argc = 0;
    int my_core_ind = -1;
    int my_core_cnt = 0;
    char buff[256];
    char allcores[256] = {0};
    CORE_CONFIG *core;

    strcpy(dpdk_argv[argc], "dkfw");
    argc++;

    if(config->nuique_name[0]){
        sprintf(dpdk_argv[argc], "--file-prefix=%s", config->nuique_name);
        argc++;
    }

    if(!config->single_process){
        sprintf(dpdk_argv[argc], "--proc-type=%s", config->process_type == PROCESS_TYPE_PRIMARY ? "primary" : "secondary");
        argc++;
    }

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core = &config->cores_pkt_process[i];
        if(core->core_enabled){
            if(core->core_is_me){
                my_core_ind = core->core_ind;
                my_core_cnt++;
            }
            sprintf(buff, "%d,", core->core_ind);
            strcat(allcores, buff);
        }else{
            break;
        }
    }

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core = &config->cores_pkt_dispatch[i];
        if(core->core_enabled){
            if(core->core_is_me){
                my_core_ind = core->core_ind;
                my_core_cnt++;
            }
            sprintf(buff, "%d,", core->core_ind);
            strcat(allcores, buff);
        }else{
            break;
        }
    }

    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        core = &config->cores_other[i];
        if(core->core_enabled){
            if(core->core_is_me){
                my_core_ind = core->core_ind;
                my_core_cnt++;
            }
            sprintf(buff, "%d,", core->core_ind);
            strcat(allcores, buff);
        }else{
            break;
        }
    }

    if(config->single_process){
        if(!allcores[0]){
            printf("Invalid all cores [%s].\n", allcores);
            return -1;
        }
        allcores[strlen(allcores) - 1] = 0;
    }else{
        if(my_core_cnt != 1 || my_core_ind < 0){
            printf("Invalid my core.\n");
            return -1;
        }
    }

    strcpy(dpdk_argv[argc], "-l");
    argc++;

    if(config->single_process){
        sprintf(dpdk_argv[argc], "%s", allcores);
    }else{
        sprintf(dpdk_argv[argc], "%d", my_core_ind);
    }
    argc++;

    if(config->number_of_channels > 0){
        strcpy(dpdk_argv[argc], "-n");
        argc++;
        sprintf(dpdk_argv[argc], "%d", config->number_of_channels);
        argc++;
    }

    if(config->socket_limit[0]){
        sprintf(dpdk_argv[argc], "--socket-limit=%s", config->socket_limit);
        argc++;
    }

    for(i=0;i<MAX_PCI_NUM;i++){
        if(config->pcis_config[i].pci_name[0]){
            sprintf(dpdk_argv[argc], "--allow=%s", config->pcis_config[i].pci_name);
            argc++;
        }
    }

    printf("rte_eal_init: ");
    for(i=0;i<argc;i++){
        printf("%s ", dpdk_argv[i]);
        argv_in[i] = dpdk_argv[i];
    }
    printf("\n");

    /* 
        根据用户的配置，组装dpdk需要的参数，并调用rte_eal_init
        具体含义见dpdk文档
    */
    ret = rte_eal_init(argc, argv_in);
    if (ret < 0) {
        printf("Error with EAL initialization\n");
        ret = -1;
    }

    return ret;
}

/*
框架初始化，用户填好config之后调用
返回0成功，其他失败
*/
int dkfw_init(DKFW_CONFIG *config)
{
    int i;
    int intf_rx_q_num;

    // 根据用户配置，自动填入相关信息
    config->cores_pkt_process_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_process[i].core_enabled){
            config->cores_pkt_process_num++;
            config->cores_pkt_process[i].core_seq = i;
            config->cores_pkt_process[i].core_role = CORE_ROLE_PKT_PROCESS;
        }else{
            break;
        }
    }
    if(!config->cores_pkt_process_num){
        printf("At lesat one process core.\n");
        goto err;
    }

    // 根据用户配置，自动填入相关信息
    config->cores_pkt_dispatch_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_pkt_dispatch[i].core_enabled){
            config->cores_pkt_dispatch_num++;
            config->cores_pkt_dispatch[i].core_seq = i;
            config->cores_pkt_dispatch[i].core_role = CORE_ROLE_PKT_DISPATCH;
        }else{
            break;
        }
    }

    // 根据用户配置，自动填入相关信息
    config->cores_other_num = 0;
    for(i=0;i<MAX_CORES_PER_ROLE;i++){
        if(config->cores_other[i].core_enabled){
            config->cores_other_num++;
            config->cores_other[i].core_seq = i;
            config->cores_other[i].core_role = CORE_ROLE_OTHER;
        }else{
            break;
        }
    }

    // 初始化dpdk本体
    if(eal_init(config) < 0){
        goto err;
    }

    // 每块网卡的rss队列数量
    if(config->cores_pkt_dispatch_num){
        // 如果有专用分发核，则rss队列数量设为分发核数，每个分发核对应一个rss队列
        intf_rx_q_num = config->cores_pkt_dispatch_num;
    }else{
        // 如果无有专用分发核，则每个业务核都有分发功能
        // rss队列数量设为业务核数，每个业务核对应一个rss队列
        intf_rx_q_num = config->cores_pkt_process_num;
    }

    // 初始化网卡
    if(interfaces_init(config, config->cores_pkt_process_num, intf_rx_q_num) < 0){
        goto err;
    }

    // 初始化各个核心
    if(cores_init(config) < 0){
        goto err;
    }

    // 初始化控制系统
    if(init_ipc_msg_pool(256) < 0){
        goto err;
    }

    if(global_init_sharemem() < 0){
        goto err;
    }

    return 0;

err:

    rte_eal_cleanup();
    return -1;
}

/*
框架退出
*/
void dkfw_exit(void)
{
    // 执行dpdk的清理函数
    rte_eal_cleanup();
}

