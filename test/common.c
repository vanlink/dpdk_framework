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

    printf("pkts:%-10llu pps:%-10llu bits:%-10llu bps:%-10llu\n", 
        pkts_now,
        interval ? ((pkts_now - pkts_last) * 1000 / interval) : 0,
        bits_now,
        interval ? ((bits_now - bits_last) * 1000 / interval) : 0
        );
}

static void ms_timer(uint64_t ms)
{
    if((ms % 5000) == 0){
        do_stat(ms);
    }
}

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
        elapsed_ms = rte_rdtsc() * 1000ULL / tsc_per_sec;
        
        if(elapsed_ms != elapsed_ms_last){
            ms_timer(elapsed_ms);
            elapsed_ms_last = elapsed_ms;
        }

        for(i=0;i<g_dkfw_interfaces_num;i++){
            rx_num = dkfw_rcv_pkt_from_interface(i, intf_rcvq_seq, pkts_burst, MAX_RCV_PKTS);
            if(rx_num){
                for(pktind=0;pktind<rx_num;pktind++){
                    pkt = pkts_burst[pktind];

                    stats_pkt_rcv_cnt++;
                    stats_pkt_rcv_bytes += rte_pktmbuf_pkt_len(pkt);
                
                    dst_core_seq = get_app_core_seq(pkt, g_pkt_process_core_num);
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

