#include <rte_ethdev.h>
extern "C"
{
    void init_args(void *to_next[8]){
    }
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {
        struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
        if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV4)
        {
            struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, (sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr)));
            
            if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
            {
                // 输出源端口和目的端口
                struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                // (int*)res[0][0][0]++;
                // printf("task1 src: %d.%d.%d.%d:%d -> ", ipv4_hdr->src_addr & 0xFF, (ipv4_hdr->src_addr >> 8) & 0xFF, (ipv4_hdr->src_addr >> 16) & 0xFF, (ipv4_hdr->src_addr >> 24) & 0xFF, ntohs(tcp_hdr->src_port));
                // printf("dst: %d.%d.%d.%d:%d\n", ipv4_hdr->dst_addr & 0xFF, (ipv4_hdr->dst_addr >> 8) & 0xFF, (ipv4_hdr->dst_addr >> 16) & 0xFF, (ipv4_hdr->dst_addr >> 24) & 0xFF, ntohs(tcp_hdr->dst_port));
            }
            else if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
            {
                // 输出源端口和目的端口
                struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                // (int*)res[0][0][1]++;
                // printf("task1 src: %d.%d.%d.%d:%d -> ", ipv4_hdr->src_addr & 0xFF, (ipv4_hdr->src_addr >> 8) & 0xFF, (ipv4_hdr->src_addr >> 16) & 0xFF, (ipv4_hdr->src_addr >> 24) & 0xFF, ntohs(udp_hdr->src_port));
                // printf("dst: %d.%d.%d.%d:%d\n", ipv4_hdr->dst_addr & 0xFF, (ipv4_hdr->dst_addr >> 8) & 0xFF, (ipv4_hdr->dst_addr >> 16) & 0xFF, (ipv4_hdr->dst_addr >> 24) & 0xFF, ntohs(udp_hdr->dst_port));
            }
            else
            {
                // printf("unknown protocol\n");
            }
        }
        else if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV6)
        {
            printf("ipv6\n");
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