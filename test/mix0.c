#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "dkfw_intf.h"
#include "dkfw_core.h"
#include "dkfw_profile.h"
#include "dkfw_ipc.h"
#include "dpdkframework.h"

extern void mix_loop(void);

static int core_func(void *arg)
{

    printf("----- dpdk core_func starts -----\n");
    fflush(stdout);
    
    mix_loop();

    return 0;
}


static DKFW_CONFIG dkfw_config;
CORE_CONFIG *dkfw_core_me = NULL;

int main(int argc, char **argv)
{
    memset(&dkfw_config, 0, sizeof(dkfw_config));

    dkfw_config.process_type = PROCESS_TYPE_PRIMARY;

    dkfw_config.cores_pkt_process[0].core_enabled = 1;
    dkfw_config.cores_pkt_process[0].core_ind = 35;
    dkfw_config.cores_pkt_process[0].core_is_me = 1;
    dkfw_config.cores_pkt_process[0].core_func_raw = core_func;
    dkfw_core_me = &dkfw_config.cores_pkt_process[0];

    dkfw_config.cores_pkt_process[1].core_enabled = 1;
    dkfw_config.cores_pkt_process[1].core_ind = 36;

    strcpy(dkfw_config.pcis_config[0].pci_name, "0000:01:00.0");
    
    if(dkfw_init(&dkfw_config) < 0){
        return -1;
    }

    printf("dpdk init done.\n");

    dkfw_start_loop_raw(NULL);

    return 0;
}

