#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <rte_launch.h>

#include "dkfw_intf.h"
#include "dkfw_core.h"
#include "dkfw_profile.h"
#include "dkfw_ipc.h"
#include "dpdkframework.h"

static DKFW_CONFIG dkfw_config;

static int main_loop(__rte_unused void *dummy)
{
    unsigned lcore_id;

    lcore_id = rte_lcore_id();

    printf("loop %d\n", lcore_id);

    while (1) {

    }

    return 0;
}

int main(int argc, char **argv)
{
    unsigned lcore_id = 0;

    memset(&dkfw_config, 0, sizeof(dkfw_config));

    dkfw_config.single_process = 1;

    dkfw_config.cores_pkt_process[0].core_enabled = 1;
    dkfw_config.cores_pkt_process[0].core_ind = 1;

    dkfw_config.cores_pkt_process[1].core_enabled = 1;
    dkfw_config.cores_pkt_process[1].core_ind = 2;

    dkfw_config.cores_pkt_dispatch[0].core_enabled = 1;
    dkfw_config.cores_pkt_dispatch[0].core_ind = 3;

    strcpy(dkfw_config.pcis_config[0].pci_name, "0000:02:02.0");
    
    if(dkfw_init(&dkfw_config) < 0){
        return -1;
    }

    printf("dpdk init done.\n");

    rte_eal_mp_remote_launch(main_loop, NULL, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
           break;
        }
    }

    return 0;
}

