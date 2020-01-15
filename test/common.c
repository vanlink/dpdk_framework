#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <arpa/inet.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_jhash.h>
#include <rte_malloc.h>

#include "dkfw_intf.h"
#include "dkfw_core.h"
#include "dkfw_profile.h"
#include "dkfw_ipc.h"
#include "dpdkframework.h"

static uint64_t tsc_per_sec;

#define MAX_RCV_PKTS 32
#define PRIME_VALUE 0xeaad8405

extern CORE_CONFIG *dkfw_core_me;

static uint64_t stats_pkt_rcv_cnt = 0;
static uint64_t stats_pkt_rcv_bytes = 0;

static uint64_t stats_pkt_rcv_cnt_last = 0;
static uint64_t stats_pkt_rcv_bytes_last = 0;

static uint64_t stats_ms_last = 0;

/*
    根据数据包选择目的业务处理核编号
    保证同一数据流的双方向始终得到同一处理核
    从而保证数据流相关内容无锁
    hash_size为业务处理核数量
    返回核编号，小于0为出错
*/
static inline int get_app_core_seq(struct rte_mbuf *m, int hash_size)
{
    char *dpdkdat = rte_pktmbuf_mtod(m, char *);
    struct rte_ether_hdr *eth;
    struct rte_ipv4_hdr *ipv4;
    struct rte_tcp_hdr *tcp;

    if(hash_size <= 1){
        return 0;
    }

    eth = (struct rte_ether_hdr *)dpdkdat;

    /*
        以以下三元组为源数据进行hash：
        (源/目的IP中较小的, 源/目的IP中较大的, 源端口+目的端口)
        这样可以保证同一流的两个方向hash值一致
    */
    if(ntohs(eth->ether_type) == RTE_ETHER_TYPE_IPV4){
        ipv4 = (struct rte_ipv4_hdr *)(dpdkdat + RTE_ETHER_HDR_LEN);
        if(ipv4->next_proto_id == IPPROTO_TCP){
            tcp = (struct rte_tcp_hdr *)((char *)ipv4 + (ipv4->version_ihl & 0x0f) * 4);

            if(htonl(ipv4->src_addr) < htonl(ipv4->dst_addr)){
                return rte_jhash_3words(htonl(ipv4->src_addr), htonl(ipv4->dst_addr), ntohs(tcp->src_port) + ntohs(tcp->dst_port), PRIME_VALUE) % hash_size;
            }else{
                return rte_jhash_3words(htonl(ipv4->dst_addr), htonl(ipv4->src_addr), ntohs(tcp->src_port) + ntohs(tcp->dst_port), PRIME_VALUE) % hash_size;
            }
        }else{
            return -1;
        }
    }else{
        return -1;
    }

    return -1;
}

// 计算并打印统计信息
static void do_stat(uint64_t ms)
{
    long long unsigned int pkts_now = (long long unsigned int)stats_pkt_rcv_cnt;
    long long unsigned int pkts_last = (long long unsigned int)stats_pkt_rcv_cnt_last;

    long long unsigned int bits_now = (long long unsigned int)stats_pkt_rcv_bytes * 8;
    long long unsigned int bits_last = (long long unsigned int)stats_pkt_rcv_bytes_last * 8;
    long long unsigned int interval = ms - stats_ms_last;

    stats_pkt_rcv_cnt_last = stats_pkt_rcv_cnt;
    stats_pkt_rcv_bytes_last = stats_pkt_rcv_bytes;
    stats_ms_last = ms;

    printf("pkts:%-12llu pps:%-12llu bits:%-15llu bps:%-15llu\n", 
        pkts_now,
        interval ? ((pkts_now - pkts_last) * 1000 / interval) : 0,
        bits_now,
        interval ? ((bits_now - bits_last) * 1000 / interval) : 0
        );
}

// 定时函数，5秒一次
static void ms_timer(uint64_t ms)
{
    if((ms % 5000) == 0){
        do_stat(ms);
    }
}

// 专用分发核主循环
void distribute_loop(void)
{
    int i, pktind;
    int rx_num;
    int intf_rcvq_seq = dkfw_core_me->core_seq;
    struct rte_mbuf *pkts_burst[MAX_RCV_PKTS];
    struct rte_mbuf *pkt;
    int dst_core_seq;
    uint64_t elapsed_ms, elapsed_ms_last = 0;

    tsc_per_sec = rte_get_tsc_hz();

    while(1){
        // 计算当前毫秒数
        elapsed_ms = rte_rdtsc() * 1000ULL / tsc_per_sec;

        // 定时器，每毫秒一次
        if(elapsed_ms != elapsed_ms_last){
            ms_timer(elapsed_ms);
            elapsed_ms_last = elapsed_ms;
        }

        for(i=0;i<g_dkfw_interfaces_num;i++){
            // 从每个物理接口的第intf_rcvq_seq个rss队列收包
            rx_num = dkfw_rcv_pkt_from_interface(i, intf_rcvq_seq, pkts_burst, MAX_RCV_PKTS);
            if(rx_num){
                for(pktind=0;pktind<rx_num;pktind++){
                    pkt = pkts_burst[pktind];

                    stats_pkt_rcv_cnt++;
                    stats_pkt_rcv_bytes += rte_pktmbuf_pkt_len(pkt);

                    // 计算目的业务处理核序号
                    dst_core_seq = get_app_core_seq(pkt, g_pkt_process_core_num);
                    // 发送给第dst_core_seq业务处理核的第intf_rcvq_seq个队列
                    if(likely(dst_core_seq >= 0)){
                        dkfw_send_pkt_to_process_core_q(dst_core_seq, intf_rcvq_seq, pkt);
                    }else{
                        rte_pktmbuf_free(pkt);
                    }
                }
            }
        }
    }

}

void app_loop(void)
{
    int i, j;
    int rx_num;
    int core_seq = dkfw_core_me->core_seq;
    struct rte_mbuf *pkts_burst[MAX_RCV_PKTS];
    struct rte_mbuf *pkt;
    uint64_t elapsed_ms, elapsed_ms_last = 0;

    tsc_per_sec = rte_get_tsc_hz();

    while(1){
        // 计算当前毫秒数
        elapsed_ms = rte_rdtsc() * 1000ULL / tsc_per_sec;

        // 定时器，每毫秒一次
        if(elapsed_ms != elapsed_ms_last){
            ms_timer(elapsed_ms);
            elapsed_ms_last = elapsed_ms;
        }

        for(i=0;i<g_pkt_distribute_core_num;i++){
            // 从本cpu的第i个收包队列收取数据包
            rx_num = dkfw_rcv_pkt_from_process_core_q(core_seq, i, pkts_burst, MAX_RCV_PKTS);
            if(!rx_num){
                continue;
            }
            for(j=0;j<rx_num;j++){
                pkt = pkts_burst[j];
                stats_pkt_rcv_cnt++;
                stats_pkt_rcv_bytes += rte_pktmbuf_pkt_len(pkt);
                rte_pktmbuf_free(pkt);
            }
        }
    }

}

void mix_loop(void)
{
    int i, j;
    int rx_num;
    int core_seq = dkfw_core_me->core_seq;
    int intf_rcvq_seq = dkfw_core_me->core_seq;
    int dst_core_seq;
    struct rte_mbuf *pkts_burst[MAX_RCV_PKTS];
    struct rte_mbuf *pkt;
    uint64_t elapsed_ms, elapsed_ms_last = 0;

    tsc_per_sec = rte_get_tsc_hz();

    while(1){
        // 计算当前毫秒数
        elapsed_ms = rte_rdtsc() * 1000ULL / tsc_per_sec;

        // 定时器，每毫秒一次
        if(elapsed_ms != elapsed_ms_last){
            ms_timer(elapsed_ms);
            elapsed_ms_last = elapsed_ms;
        }

        for(i=0;i<g_dkfw_interfaces_num;i++){
            // 从每个物理接口的第intf_rcvq_seq个rss队列收包
            rx_num = dkfw_rcv_pkt_from_interface(i, intf_rcvq_seq, pkts_burst, MAX_RCV_PKTS);
            if(rx_num){
                for(j=0;j<rx_num;j++){
                    pkt = pkts_burst[j];

                    // 计算目的业务处理核序号
                    dst_core_seq = get_app_core_seq(pkt, g_pkt_process_core_num);
                    // 发送给第dst_core_seq业务处理核的第intf_rcvq_seq个队列
                    if(likely(dst_core_seq >= 0)){
                        dkfw_send_pkt_to_process_core_q(dst_core_seq, intf_rcvq_seq, pkt);
                    }else{
                        rte_pktmbuf_free(pkt);
                    }
                }
            }
        }

        for(i=0;i<g_pkt_process_core_num;i++){
            // 从本cpu的第i个收包队列收取数据包
            rx_num = dkfw_rcv_pkt_from_process_core_q(core_seq, i, pkts_burst, MAX_RCV_PKTS);
            if(!rx_num){
                continue;
            }
            for(j=0;j<rx_num;j++){
                pkt = pkts_burst[j];
                stats_pkt_rcv_cnt++;
                stats_pkt_rcv_bytes += rte_pktmbuf_pkt_len(pkt);
                rte_pktmbuf_free(pkt);
            }
        }
    }

}

