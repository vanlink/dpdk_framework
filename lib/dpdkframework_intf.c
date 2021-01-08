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
#include <rte_mempool.h>

#include "dkfw_intf.h"

static DKFW_INTF g_dkfw_interfaces[MAX_PCI_NUM];
int g_dkfw_interfaces_num;

// packet pool, one for each dispatch core
static struct rte_mempool *g_eth_rxq[MAX_CORES_PER_ROLE] = { NULL };

void dkfw_get_pkt_pool_name(int dispatch_core_ind, char *buff)
{
    sprintf(buff, "dkfwnicpkts%d", dispatch_core_ind);
}

/* 检查链路up状态 */
static void check_port_link_status(int port_ind)
{
    int i;
    struct rte_eth_link link;

    printf("Checking intf %d status ... \n", port_ind);

    for(i=0;i<10;i++){
        
        rte_delay_ms(1000);
        
        memset(&link, 0, sizeof(link));
        rte_eth_link_get_nowait(port_ind, &link);

        // 检查并打印链路状态信息
        if (link.link_status) {
            printf("Port Link Up - %u Mbps - %s\n", (unsigned)link.link_speed, (link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex") : ("half-duplex\n"));
            return;
        }
    }

    printf("Port Link Down\n");
}

// 网卡对称rss哈希相关参数值
#define RSS_HASH_KEY_LENGTH 40
static uint8_t hash_key[RSS_HASH_KEY_LENGTH] = {
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
};

static int get_max_interface_rcv_pkt_len(int config_len)
{
    if(config_len){
        if(config_len < RTE_ETHER_MIN_LEN){
            return RTE_ETHER_MIN_LEN;
        }else if (config_len > MAX_JUMBO_FRAME_SIZE){
            return MAX_JUMBO_FRAME_SIZE;
        }else{
            return config_len;
        }
    }else{
        return RTE_ETHER_MAX_LEN;
    }
}

static int sym_hash_enable(int port_id, uint32_t ftype, enum rte_eth_hash_function function)
{
    struct rte_eth_hash_filter_info info;
    int ret = 0;
    uint32_t idx = 0;
    uint32_t offset = 0;

    memset(&info, 0, sizeof(info));

    ret = rte_eth_dev_filter_supported(port_id, RTE_ETH_FILTER_HASH);
    if (ret < 0) {
        printf("RTE_ETH_FILTER_HASH not supported on port: %d", port_id);
        return ret;
    }

    info.info_type = RTE_ETH_HASH_FILTER_GLOBAL_CONFIG;
    info.info.global_conf.hash_func = function;

    idx = ftype / UINT64_BIT;
    offset = ftype % UINT64_BIT;
    info.info.global_conf.valid_bit_mask[idx] |= (1ULL << offset);
    info.info.global_conf.sym_hash_enable_mask[idx] |= (1ULL << offset);

    ret = rte_eth_dev_filter_ctrl(port_id, RTE_ETH_FILTER_HASH, RTE_ETH_FILTER_SET, &info);
    if (ret < 0){
        printf("Cannot set global hash configurations on port %u", port_id);
        return ret;
    }

    return 0;
}

static int sym_hash_set(int port_id, int enable)
{
    int ret = 0;
    struct rte_eth_hash_filter_info info;

    memset(&info, 0, sizeof(info));

    ret = rte_eth_dev_filter_supported(port_id, RTE_ETH_FILTER_HASH);
    if (ret < 0) {
        printf("RTE_ETH_FILTER_HASH not supported on port: %d", port_id);
        return ret;
    }

    info.info_type = RTE_ETH_HASH_FILTER_SYM_HASH_ENA_PER_PORT;
    info.info.enable = enable;
    ret = rte_eth_dev_filter_ctrl(port_id, RTE_ETH_FILTER_HASH, RTE_ETH_FILTER_SET, &info);

    if (ret < 0) {
        printf("Cannot set symmetric hash enable per port on port %u", port_id);
        return ret;
    }

    return 0;
}

/*
    初始化一个网卡
    返回0成功，其他失败
*/
static int interfaces_init_one(PCI_CONFIG *config, DKFW_INTF *dkfw_intf, int txq_num, int rxq_num, int max_rx_pkt_len)
{
    int ret, i;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_conf port_conf;
    struct rte_eth_txconf txconf;
    struct rte_eth_rxconf rxconf;
    int port_socket_id = SOCKET_ID_ANY;
    uint16_t nb_txd;
    uint16_t nb_rxd;
    int port_ind = dkfw_intf->intf_seq;

    printf("INIT intf %d ...\n", port_ind);

    // 从dpdk获取网卡硬件基本信息
    if(rte_eth_dev_info_get(port_ind, &dev_info)){
        printf("failed to rte_eth_dev_info_get.\n");
        return -1;
    }
    
    printf("interface max_tx_queues = %d\n", dev_info.max_tx_queues);
    printf("interface max_rx_queues = %d\n", dev_info.max_rx_queues);
    printf("interface max tx desc = %d\n", dev_info.tx_desc_lim.nb_max);
    printf("interface max rx desc = %d\n", dev_info.rx_desc_lim.nb_max);

    nb_txd = config->nic_tx_desc ? config->nic_tx_desc : dev_info.tx_desc_lim.nb_max;
    nb_rxd = config->nic_rx_desc ? config->nic_rx_desc : dev_info.rx_desc_lim.nb_max;

    // 设置网卡配型，万兆或四万兆
    if(strstr(dev_info.driver_name, "i40e")){
        dkfw_intf->nic_type = NETCARD_TYPE_I40E;
    }

    // 配置网卡只能由主进程进行
    if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
        return 0;
    }

    // 获取网卡连接的numa id
    i = rte_eth_dev_socket_id(port_ind);
    printf("interface %d is on socket %d.\n", port_ind, i);

    // 禁用网卡
    rte_eth_dev_set_link_down(port_ind);
    // rte_eth_dev_stop(port_ind);

    // 检查网卡的最大发送接收队列是否满足配置
    if(dev_info.max_tx_queues < txq_num){
        printf("eth interface TX queues too few.\n");
        return -1;
    }

    if(dev_info.max_rx_queues < rxq_num){
        printf("eth interface RX queues too few.\n");
        return -1;
    }

    // 开始设置网卡参数
    memset(&port_conf, 0, sizeof(port_conf));

    // 最大接收包长
    port_conf.rxmode.max_rx_pkt_len = max_rx_pkt_len;

    printf("interface %d max_rx_pkt_len %d\n", port_ind, port_conf.rxmode.max_rx_pkt_len);

    // 网卡硬件快速释放特性
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE){
        printf("offload DEV_TX_OFFLOAD_MBUF_FAST_FREE\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    // 网卡硬件发送ipv4检验和计算
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_IPV4_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
    }
    // 网卡硬件发送tcp检验和计算
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_TCP_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_TCP_CKSUM;
    }
    // 网卡硬件发送udp检验和计算
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM) {
        printf("offload DEV_TX_OFFLOAD_UDP_CKSUM\n");
        port_conf.txmode.offloads |= DEV_TX_OFFLOAD_UDP_CKSUM;
    }

    if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_IPV4_CKSUM) {
        printf("offload DEV_RX_OFFLOAD_IPV4_CKSUM\n");
        port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_IPV4_CKSUM;
    }

    if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_TCP_CKSUM) {
        printf("offload DEV_RX_OFFLOAD_TCP_CKSUM\n");
        port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_TCP_CKSUM;
    }

    if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_UDP_CKSUM) {
        printf("offload DEV_RX_OFFLOAD_UDP_CKSUM\n");
        port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_UDP_CKSUM;
    }

    if(config->nic_hw_strip_vlan){
        if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_VLAN_STRIP) {
            printf("offload DEV_RX_OFFLOAD_VLAN_STRIP\n");
            port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_VLAN_STRIP;
        }
    }

    if(config->nic_hw_strip_qinq){
        if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_QINQ_STRIP) {
            printf("offload DEV_RX_OFFLOAD_QINQ_STRIP\n");
            port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_QINQ_STRIP;
        }
    }

    if(port_conf.rxmode.max_rx_pkt_len > RTE_ETHER_MAX_LEN){
        if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_JUMBO_FRAME) {
            printf("offload DEV_RX_OFFLOAD_JUMBO_FRAME\n");
            port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_JUMBO_FRAME;
        }
    }

    // 设置对称RSS队列
    // 可使一条流的两个方向的包，始终分配到同一个rss队列
    if(rxq_num > 1){
        port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
        port_conf.rx_adv_conf.rss_conf.rss_hf = (ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP) & dev_info.flow_type_rss_offloads;
        if(dkfw_intf->nic_type == NETCARD_TYPE_I40E){
            printf("Set Symmetric RSS for 40G netcard step 1.\n");
        }else{
            printf("Set Symmetric RSS for 10G netcard.\n");
            port_conf.rx_adv_conf.rss_conf.rss_key = hash_key;
            port_conf.rx_adv_conf.rss_conf.rss_key_len = RSS_HASH_KEY_LENGTH;
        }
    }

    // 使能混杂模式
    rte_eth_promiscuous_enable(port_ind);
    // 接收组播包
    rte_eth_allmulticast_enable(port_ind);

    // 配置网卡
    ret = rte_eth_dev_configure(port_ind, rxq_num, txq_num, &port_conf);
    if (ret) {
        printf("rte_eth_dev_configure err\n");
        return -1;
    }

    if(rxq_num > 1){
        if(dkfw_intf->nic_type == NETCARD_TYPE_I40E){
            printf("Set Symmetric RSS for 40G netcard step 2.\n");
            if(sym_hash_enable(port_ind, RTE_ETH_FLOW_NONFRAG_IPV4_TCP, RTE_ETH_HASH_FUNCTION_TOEPLITZ) < 0){
                return -1;
            }
            if(sym_hash_enable(port_ind, RTE_ETH_FLOW_NONFRAG_IPV4_UDP, RTE_ETH_HASH_FUNCTION_TOEPLITZ) < 0){
                return -1;
            }
            if(sym_hash_set(port_ind, 1) < 0){
                return -1;
            }
        }
    }

    // 设置网卡发送和接收描述符的数量为最大
    if (rte_eth_dev_adjust_nb_rx_tx_desc(port_ind, &nb_rxd, &nb_txd)){
        printf("rte_eth_dev_adjust_nb_rx_tx_desc err\n");
        return -1;
    }

    printf("Set port desc num rx=%d tx=%d\n", nb_rxd, nb_txd);

    // 设置每个接收队列
    for (i=0; i<rxq_num;i++) {
        struct rte_eth_rxq_info qinfo;

        rxconf = dev_info.default_rxconf;
        rxconf.offloads = port_conf.rxmode.offloads;

        // 设置一个接收队列
        ret = rte_eth_rx_queue_setup(port_ind, i, nb_rxd, port_socket_id, &rxconf, g_eth_rxq[i]);
        if (ret) {
            printf("rte_eth_rx_queue_setup %d err\n", i);
            return -1;
        }
        // 验证接收队列设置是否成功
        if(rte_eth_rx_queue_info_get(port_ind, i, &qinfo)){
            printf("rte_eth_rx_queue_setup %d get info err.\n", i);
            return -1;
        }
    }

    // 设置每个发送队列
    for (i=0; i<txq_num;i++) {
        struct rte_eth_txq_info qinfo;
        
        txconf = dev_info.default_txconf;
        txconf.offloads = port_conf.txmode.offloads;
        // 设置一个发送队列
        ret = rte_eth_tx_queue_setup(port_ind, i, nb_txd, port_socket_id, &txconf);
        if (ret) {
            printf("rte_eth_tx_queue_setup %d err\n", i);
            return -1;
        }
        // 验证发送队列设置是否成功
        if(rte_eth_tx_queue_info_get(port_ind, i, &qinfo)){
            printf("rte_eth_tx_queue_setup %d get info err.\n", i);
            return -1;
        }
    }

    // 点亮网卡指示灯
    rte_eth_led_on(port_ind);
    // 使能网卡，链路UP
    rte_eth_dev_set_link_up(port_ind);

    // 开始收包
    ret = rte_eth_dev_start(port_ind);
    if (ret) {
        printf("rte_eth_dev_start err\n");
        return -1;
    }

    // 检测链路up状态
    check_port_link_status(port_ind);

    return 0;
}

/*
    初始化所有网卡
    给每个网卡分配txq_num个发送队列
    rxq_num个rss接收队列
    返回0成功，其他失败
*/
int interfaces_init(DKFW_CONFIG *config, int txq_num, int rxq_num)
{
    int i;
    int pkt_cnt = 0, pkt_size= 0, config_pkt_len = 0;
    char buff[32];
    int disp_core_cnt;

    // 从dpdk获取连接的网卡数
    g_dkfw_interfaces_num = rte_eth_dev_count_avail();

    printf("We have %d interfaces.\n", g_dkfw_interfaces_num);
    printf("Init each intf txq=%d rxq=%d.\n", txq_num, rxq_num);
    
    if(g_dkfw_interfaces_num < 1 || g_dkfw_interfaces_num > MAX_PCI_NUM){
        return -1;
    }

    memset(g_dkfw_interfaces, 0, sizeof(g_dkfw_interfaces));

    if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
        disp_core_cnt = config->cores_pkt_dispatch_num ? config->cores_pkt_dispatch_num : config->cores_pkt_process_num;

        if(config->nic_rx_pktbuf_cnt){
            pkt_cnt = config->nic_rx_pktbuf_cnt;
        }else{
            pkt_cnt = 100000;
        }
        pkt_cnt = pkt_cnt / disp_core_cnt;
        config_pkt_len = get_max_interface_rcv_pkt_len(config->nic_max_rx_pkt_len);
        pkt_size = RTE_MBUF_DEFAULT_BUF_SIZE;
        if(config_pkt_len > RTE_ETHER_MAX_LEN){
            pkt_size += ((config_pkt_len - RTE_ETHER_MAX_LEN) + 32);
        }

        for(i=0;i<disp_core_cnt;i++){
            dkfw_get_pkt_pool_name(i, buff);
            printf("Pkt pool [%d] pktsize=%d pktcnt=%d\n", i, pkt_size, pkt_cnt);
            g_eth_rxq[i] = rte_pktmbuf_pool_create(buff, pkt_cnt, 256, RTE_MBUF_PRIV_ALIGN * 4, pkt_size, SOCKET_ID_ANY);
            if(!g_eth_rxq[i]){
                printf("rte_pktmbuf_pool_create for intf rx q err\n");
                return -1;
            }
        }
    }

    // 逐个初始化网卡
    for(i=0;i<g_dkfw_interfaces_num;i++){
        g_dkfw_interfaces[i].intf_seq = i;
        if(interfaces_init_one(&config->pcis_config[i], &g_dkfw_interfaces[i], txq_num, rxq_num, config_pkt_len) < 0){
            return -1;
        }
    }

    return 0;
}

/*
    从编号为intf_seq的网卡的第q_num个rss队列收包
    最大接收max_pkts_num个包
    pkts_burst为包的缓冲区
    返回实际接收到的包数
*/
int dkfw_rcv_pkt_from_interface(int intf_seq, int q_num, struct rte_mbuf **pkts_burst, int max_pkts_num)
{
    int rx;
    
    rx = rte_eth_rx_burst(intf_seq, q_num, pkts_burst, max_pkts_num);
    g_dkfw_interfaces[intf_seq].stats_rcv_pkts_cnt[q_num] += rx;

    return rx;
}

// 从每个网口的第q_num个队列收到的包数
void dkfw_pkt_rcv_from_interfaces_stat(int q_num, uint64_t *stats)
{
    int i;

    for(i=0;i<g_dkfw_interfaces_num;i++){
        stats[i] = g_dkfw_interfaces[i].stats_rcv_pkts_cnt[q_num];
    }
}

// 第intf_seq接口的每个收包队列中的当前包数
int dkfw_pkt_mbuf_pool_stat(int seq, uint64_t *stats_inuse, uint64_t *stats_ava)
{
    int i = 0;
    struct rte_mempool *mp;
    char buff[64];

    dkfw_get_pkt_pool_name(seq, buff);
    mp = rte_mempool_lookup(buff);
    if(mp){
        stats_ava[i] = rte_mempool_avail_count(mp);
        stats_inuse[i] = rte_mempool_in_use_count(mp);
    }else{
        stats_ava[i] = 99999999;
        stats_inuse[i] = 99999999;
    }

    return 0;
}

// read nic stats form hw, add it to local stats, and clear hw stats
int dkfw_update_intf_stats(int nic_seq)
{
    struct rte_eth_stats eth_stats;

    memset(&eth_stats, 0, sizeof(eth_stats));

    if(rte_eth_stats_get(nic_seq, &eth_stats) < 0){
        return -1;
    }

    g_dkfw_interfaces[nic_seq].nic_stats.ipackets += eth_stats.ipackets;
    g_dkfw_interfaces[nic_seq].nic_stats.opackets += eth_stats.opackets;
    g_dkfw_interfaces[nic_seq].nic_stats.ibytes += eth_stats.ibytes;
    g_dkfw_interfaces[nic_seq].nic_stats.obytes += eth_stats.obytes;
    g_dkfw_interfaces[nic_seq].nic_stats.imissed += eth_stats.imissed;
    g_dkfw_interfaces[nic_seq].nic_stats.ierrors += eth_stats.ierrors;
    g_dkfw_interfaces[nic_seq].nic_stats.oerrors += eth_stats.oerrors;
    g_dkfw_interfaces[nic_seq].nic_stats.rx_nombuf += eth_stats.rx_nombuf;

    if(rte_eth_stats_reset(nic_seq) < 0){
        return -1;
    }

    return 0;
}

