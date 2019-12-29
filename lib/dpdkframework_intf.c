#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_string_fns.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#include "dkfw_intf.h"

static DKFW_INTF g_dkfw_interfaces[MAX_INTERFACE_NUM];
int g_dkfw_interfaces_num;

static void check_port_link_status(int port_ind)
{
    int i;
    struct rte_eth_link link;

    printf("Checking intf %d status ... \n", port_ind);

    for(i=0;i<10;i++){
        
        rte_delay_ms(1000);
        
        memset(&link, 0, sizeof(link));
        rte_eth_link_get_nowait(port_ind, &link);
        
        if (link.link_status) {
            printf("Port Link Up - %u Mbps - %s\n", (unsigned)link.link_speed, (link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex") : ("half-duplex\n"));
            return;
        }
    }

    printf("Port Link Down\n");
}

static int interfaces_init_one(DKFW_INTF *dkfw_intf, int txq_num, int rxq_num)
{
    int ret, i;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_conf port_conf;
    struct rte_eth_txconf txconf;
    struct rte_eth_rxconf rxconf;
    int port_socket_id = SOCKET_ID_ANY;
    uint16_t nb_txd;
    uint16_t nb_rxd;
    struct rte_mempool *eth_rxq = NULL;
    char buff[1024];
    int port_ind = dkfw_intf->intf_ind;
    
    rte_eth_dev_info_get(port_ind, &dev_info);

    printf("INIT intf %d ...\n", port_ind);
    
    printf("interface max_tx_queues = %d\n", dev_info.max_tx_queues);
    printf("interface max_rx_queues = %d\n", dev_info.max_rx_queues);
    printf("interface max tx desc = %d\n", dev_info.tx_desc_lim.nb_max);
    printf("interface max rx desc = %d\n", dev_info.rx_desc_lim.nb_max);

    nb_txd = dev_info.tx_desc_lim.nb_max;
    nb_rxd = dev_info.rx_desc_lim.nb_max;

    if(strstr(dev_info.driver_name, "i40e")){
        dkfw_intf->nic_type = NETCARD_TYPE_I40E;
    }

    if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
        return 0;
    }

    rte_eth_dev_set_link_down(port_ind);
    // rte_eth_dev_stop(port_ind);

    if(dev_info.max_tx_queues < txq_num){
        printf("eth interface TX queues too few.\n");
        return -1;
    }

    if(dev_info.max_rx_queues < rxq_num){
        printf("eth interface RX queues too few.\n");
        return -1;
    }

    memset(&port_conf, 0, sizeof(port_conf));

    port_conf.rxmode.max_rx_pkt_len = 1518;

    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE){
        printf("offload DEV_TX_OFFLOAD_MBUF_FAST_FREE\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
    }
    
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_IPV4_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
    }
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_TCP_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_TCP_CKSUM;
    }
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_UDP_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_UDP_CKSUM;
    }

    if(rxq_num > 1){
        port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
        port_conf.rx_adv_conf.rss_conf.rss_hf = (ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP) & dev_info.flow_type_rss_offloads;
    }

    rte_eth_promiscuous_enable(port_ind);
    rte_eth_allmulticast_enable(port_ind);

    ret = rte_eth_dev_configure(port_ind, rxq_num, txq_num, &port_conf);
    if (ret) {
        printf("rte_eth_dev_configure err\n");
        return -1;
    }

    if (rte_eth_dev_adjust_nb_rx_tx_desc(port_ind, &nb_rxd, &nb_txd)){
        printf("rte_eth_dev_adjust_nb_rx_tx_desc err\n");
        return -1;
    }

    printf("Set port desc num rx=%d tx=%d\n", nb_rxd, nb_txd);

    for (i=0; i<rxq_num;i++) {
        struct rte_eth_rxq_info qinfo;

        rxconf = dev_info.default_rxconf;
        rxconf.offloads = port_conf.rxmode.offloads;

        sprintf(buff, "intfrx-%d-%d", port_ind, i);
        eth_rxq = rte_pktmbuf_pool_create(buff, 8192, 512, RTE_MBUF_PRIV_ALIGN, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);
        if(!eth_rxq){
            printf("rte_pktmbuf_pool_create for intf rx q err\n");
            return -1;
        }

        ret = rte_eth_rx_queue_setup(port_ind, i, nb_rxd, port_socket_id, &rxconf, eth_rxq);
        if (ret) {
            printf("rte_eth_rx_queue_setup %d err\n", i);
            return -1;
        }

        if(rte_eth_rx_queue_info_get(port_ind, i, &qinfo)){
            printf("rte_eth_rx_queue_setup %d get info err.\n", i);
            return -1;
        }
    }
    
    for (i=0; i<txq_num;i++) {
        struct rte_eth_txq_info qinfo;
        
        txconf = dev_info.default_txconf;
        txconf.offloads = port_conf.txmode.offloads;

        ret = rte_eth_tx_queue_setup(port_ind, i, nb_txd, port_socket_id, &txconf);
        if (ret) {
            printf("rte_eth_tx_queue_setup %d err\n", i);
            return -1;
        }

        if(rte_eth_tx_queue_info_get(port_ind, i, &qinfo)){
            printf("rte_eth_tx_queue_setup %d get info err.\n", i);
            return -1;
        }
    }

    rte_eth_led_on(port_ind);
    rte_eth_dev_set_link_up(port_ind);

    ret = rte_eth_dev_start(port_ind);
    if (ret) {
        printf("rte_eth_dev_start err\n");
        return -1;
    }

    check_port_link_status(port_ind);

    return 0;
}

int interfaces_init(int txq_num, int rxq_num)
{
    int i;
    
    g_dkfw_interfaces_num = rte_eth_dev_count_avail();

    printf("We have %d interfaces.\n", g_dkfw_interfaces_num);
    printf("Init each intf txq=%d rxq=%d.\n", txq_num, rxq_num);
    
    if(g_dkfw_interfaces_num < 1 || g_dkfw_interfaces_num > MAX_INTERFACE_NUM){
        return -1;
    }

    memset(g_dkfw_interfaces, 0, sizeof(g_dkfw_interfaces));

    for(i=0;i<g_dkfw_interfaces_num;i++){
        g_dkfw_interfaces[i].intf_ind = i;
        if(interfaces_init_one(&g_dkfw_interfaces[i], txq_num, rxq_num) < 0){
            return -1;
        }
    }

    return 0;
}

int dkfw_rcv_pkt_from_interface(int intf_seq, int q_num, struct rte_mbuf **pkts_burst, int max_pkts_num)
{
    int rx;
    
    rx = rte_eth_rx_burst(intf_seq, q_num, pkts_burst, max_pkts_num);
    g_dkfw_interfaces[intf_seq].stats_rcv_pkts_cnt[q_num] += rx;

    return rx;
}

