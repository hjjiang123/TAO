/*
reseamble tcp flow and obtain the length of flows, then publish the result into kafka cluster' data
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <rte_ethdev.h>
#include "kafka_p_c.h"

#define MAX_FLOWS 1000
#define MAX_BUFFER_SIZE 1048576  // 1MB
#define MAX_OUT_OF_ORDER_SEGMENTS 100
#define FLOW_TIMEOUT 150  // 流超时
#define CLEANUP_INTERVAL 60  // 1分钟清理一次
#define MAX_PACKET_LENGTHS 128  // 存储每个流的前128个报文长度

extern "C"
{
    struct one_result{
        uint32_t src_ip;
        uint32_t dst_ip;
        uint16_t src_port;
        uint16_t dst_port;
        uint32_t len;
    };
    typedef struct {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint16_t src_port;
        uint16_t dst_port;
    } flow_key_t;

    typedef struct {
        uint32_t seq;
        uint32_t len; // 假设最大MTU为1500字节
    } segment_t;

    typedef struct {
        flow_key_t key;
        uint32_t next_seq;
        uint32_t buffer_size;
        // int packet_lengths[MAX_PACKET_LENGTHS];
        // int length_count;
        segment_t out_of_order_queue[MAX_OUT_OF_ORDER_SEGMENTS];
        int queue_start;
        int queue_end;
        int queue_size;
        time_t last_seen;
        uint8_t fin_received : 1;
        uint8_t fin_sent : 1;
    } tcp_flow_t;

    tcp_flow_t flows[MAX_FLOWS];
    int flow_count = 0;
    time_t last_cleanup_time = 0;

    // 序列号比较函数，考虑回绕
    int seq_compare(uint32_t seq1, uint32_t seq2) {
        return (int)((seq1 - seq2) & 0x80000000) ? -1 : (seq1 == seq2) ? 0 : 1;
    }

    tcp_flow_t* find_or_create_flow(flow_key_t *key) {
        for (int i = 0; i < flow_count; i++) {
            if (memcmp(&flows[i].key, key, sizeof(flow_key_t)) == 0) {
                return &flows[i];
            }
        }
        
        if (flow_count >= MAX_FLOWS) {
            return NULL;
        }
        
        tcp_flow_t *new_flow = &flows[flow_count++];
        memcpy(&new_flow->key, key, sizeof(flow_key_t));
        new_flow->next_seq = 0;
        new_flow->buffer_size = 0;
        new_flow->queue_start = 0;
        new_flow->queue_end = 0;
        new_flow->queue_size = 0;
        new_flow->last_seen = time(NULL);
        new_flow->fin_received = 0;
        new_flow->fin_sent = 0;
        
        return new_flow;
    }

    void insert_out_of_order_segment(tcp_flow_t *flow, uint32_t seq, uint32_t len) {
        if (flow->queue_size >= MAX_OUT_OF_ORDER_SEGMENTS) {
            return;  // 队列已满，丢弃新段
        }

        int insert_pos = flow->queue_end;
        int prev_pos = (insert_pos - 1 + MAX_OUT_OF_ORDER_SEGMENTS) % MAX_OUT_OF_ORDER_SEGMENTS;

        while (flow->queue_size > 0 && seq_compare(seq, flow->out_of_order_queue[prev_pos].seq) < 0) {
            flow->out_of_order_queue[insert_pos] = flow->out_of_order_queue[prev_pos];
            insert_pos = prev_pos;
            prev_pos = (prev_pos - 1 + MAX_OUT_OF_ORDER_SEGMENTS) % MAX_OUT_OF_ORDER_SEGMENTS;
        }

        flow->out_of_order_queue[insert_pos].seq = seq;
        flow->out_of_order_queue[insert_pos].len = len;

        flow->queue_end = (flow->queue_end + 1) % MAX_OUT_OF_ORDER_SEGMENTS;
        flow->queue_size++;
    }

    void process_in_order_data(tcp_flow_t *flow, uint32_t len) {
        if (flow->buffer_size + len <= MAX_BUFFER_SIZE) {
            flow->buffer_size += len;
            flow->next_seq += len;
            // if(flow->length_count < MAX_PACKET_LENGTHS){
            //     flow->packet_lengths[flow->length_count++] = len;
            // }
        }
    }

    void process_out_of_order_queue(tcp_flow_t *flow) {
        while (flow->queue_size > 0 && 
            seq_compare(flow->out_of_order_queue[flow->queue_start].seq, flow->next_seq) <= 0) {
            segment_t *segment = &flow->out_of_order_queue[flow->queue_start];

            if (segment->seq == flow->next_seq) {
                process_in_order_data(flow, segment->len);
            }

            flow->queue_start = (flow->queue_start + 1) % MAX_OUT_OF_ORDER_SEGMENTS;
            flow->queue_size--;
        }
    }

    void cleanup_flows(kafka_params *k_params) {
        time_t current_time = time(NULL);
        if (current_time - last_cleanup_time < CLEANUP_INTERVAL) {
            return;
        }
        
        last_cleanup_time = current_time;
        
        int i = 0;
        while (i < flow_count) {
            tcp_flow_t *flow = &flows[i];
            if ((current_time - flow->last_seen > FLOW_TIMEOUT) || 
                (flow->fin_received && flow->fin_sent)) {
                // 移除流
                if (i < flow_count - 1) {
                    memcpy(&flows[i], &flows[flow_count - 1], sizeof(tcp_flow_t));
                }
                printf("task tcp_cons src: %d.%d.%d.%d:%d -> ", flow->key.src_ip & 0xFF, (flow->key.src_ip >> 8) & 0xFF, (flow->key.src_ip >> 16) & 0xFF, (flow->key.src_ip >> 24) & 0xFF, flow->key.src_port);
                printf("dst: %d.%d.%d.%d:%d", flow->key.dst_ip & 0xFF, (flow->key.dst_ip >> 8) & 0xFF, (flow->key.dst_ip >> 16) & 0xFF, (flow->key.dst_ip >> 24) & 0xFF, flow->key.dst_port);
                printf(" len: %d\n", flow->buffer_size);
                ResultData *res = getFreePointer(k_params->res_ring);
                if(res == NULL){
                    printf("task tcp_cons res is NULL\n");
                }else{
                    printf("task tcp_cons res is not NULL\n");
                    res->size = sizeof(struct one_result);
                    struct one_result *one_res= (struct one_result *)res->data;
                    one_res->src_ip = flow->key.src_ip;
                    one_res->dst_ip = flow->key.dst_ip;
                    one_res->src_port = flow->key.src_port;
                    one_res->dst_port = flow->key.dst_port;
                    one_res->len = flow->buffer_size;
                    k_params->dataPtr = res;
                    produce_message(k_params);
                }
                flow_count--;
            } else {
                i++;
            }
        }
    }

    void process_segment(flow_key_t *key, uint32_t seq, uint32_t len, uint8_t flags,kafka_params *k_params) {
        tcp_flow_t *flow = find_or_create_flow(key);
        if (!flow) {
            return;
        }
        
        flow->last_seen = time(NULL);
        
        if (flags & 0x01) {  // FIN flag
            flow->fin_received = 1;
        }
        
        if (seq_compare(seq, flow->next_seq) < 0) {
            uint32_t offset = flow->next_seq - seq;
            if (offset >= len) {
                return;
            }
            seq += offset;
            len -= offset;
        }
        
        if (seq == flow->next_seq) {
            process_in_order_data(flow, len);
            process_out_of_order_queue(flow);
        } else if (seq_compare(seq, flow->next_seq) > 0) {
            insert_out_of_order_segment(flow, seq, len);
        }
        cleanup_flows(k_params);
    }
    void init_args(void *to_next[8]){
    }

    // allocate one result data structure, required by the framework
    int allocate_one_result(void *one_res){
        one_result *res = (one_result *)one_res;
        res = (one_result *)malloc(sizeof(one_result));
        return sizeof(one_result);
    }//TODO:加上打印函数，查看是否发布结果消息值kafka集群
    
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8], kafka_params *k_params)
    {
        struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
        struct rte_vlan_hdr *vlan_hdr;
        if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_VLAN)
        {
            vlan_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_vlan_hdr *, sizeof(struct rte_ether_hdr));
        }
        if (ntohs(vlan_hdr->eth_proto) == RTE_ETHER_TYPE_IPV4)
        {
            struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, (sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr)));
            if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
            {
                // 输出源端口和目的端口
                struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr) + sizeof(struct rte_ipv4_hdr));
                flow_key_t key={ipv4_hdr->src_addr,ipv4_hdr->dst_addr,tcp_hdr->src_port,tcp_hdr->dst_port};
                process_segment(&key, ntohl(tcp_hdr->sent_seq), rte_pktmbuf_pkt_len(pkt) - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr) - sizeof(struct rte_tcp_hdr), tcp_hdr->tcp_flags,k_params);
                
            }
        }
        return -1;
    }
    
    float evaluate_task_cpu(float c, float x, float v)
    {
        return x*v*c;
    }
    float evaluate_task_mem(float x, float v)
    {
        return x*v;
    }
    float g_cpu(float x, unsigned long long C)
    {
        return x*C;
    }
}