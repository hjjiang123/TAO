#include <rte_ethdev.h>
#define USER_ARG_THETA 1e-4
extern "C"
{
    void init_args(void *to_next[8]){
        *(unsigned int *)to_next[4] = 0; // packets count
        *(unsigned int *)to_next[5] = 0; // heavy hitters count
    }
    unsigned int os_dietz_thorup32(unsigned int src_ip,unsigned int dst_ip, unsigned short src_port,unsigned short dst_port, unsigned int bins, unsigned int a, unsigned int b){
        return ((unsigned int) ((a*(src_ip>>16+dst_ip>>16+src_port+dst_port)+b) >> 16)) % bins;
    }
    int query_sketch(unsigned int src_ip,unsigned int dst_ip, unsigned short src_port,unsigned short dst_port, char ****res, int rownum,int bucketnum,int bucketsize,int countersize){
        int index;
        int min = 0x7fffffff;
        for(int i=0;i<rownum;i++){
            index = os_dietz_thorup32(src_ip,dst_ip,src_port,dst_port,bucketnum,i,i);
            if(res[i][index][0][0] < min){
                min = res[i][index][0][0];
            }
        }
        return min;
    }
    void update_sketch(unsigned int src_ip,unsigned int dst_ip, unsigned short src_port,unsigned short dst_port, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *min_arg){
        int index;
        unsigned int min = 0x7fffffff;
        for(int i=0;i<rownum;i++){
            index = os_dietz_thorup32(src_ip,dst_ip,src_port,dst_port,bucketnum,i,i);
            *(int *)res[i][index][0] += 1;
            if (res[i][index][0][0] < min)
            {
                min = res[i][index][0][0];
            }
        }
        *(int *)min_arg = min;
    }
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {
        struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
        if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV4)
        {
            struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, (sizeof(struct rte_ether_hdr)));
            if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
            {
                struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                *(unsigned int *)to_next[0] = ipv4_hdr->src_addr;
                *(unsigned int *)to_next[1] = ipv4_hdr->dst_addr;
                *(unsigned short *)to_next[2] = tcp_hdr->src_port;
                *(unsigned short *)to_next[3] = tcp_hdr->dst_port;
                *(unsigned int *)to_next[4]  += 1;
                // printf("task2 src: %d.%d.%d.%d:%d -> ", ipv4_hdr->src_addr & 0xFF, (ipv4_hdr->src_addr >> 8) & 0xFF, (ipv4_hdr->src_addr >> 16) & 0xFF, (ipv4_hdr->src_addr >> 24) & 0xFF, ntohs(tcp_hdr->src_port));
                // printf("task2 dst: %d.%d.%d.%d:%d\n", ipv4_hdr->dst_addr & 0xFF, (ipv4_hdr->dst_addr >> 8) & 0xFF, (ipv4_hdr->dst_addr >> 16) & 0xFF, (ipv4_hdr->dst_addr >> 24) & 0xFF, ntohs(tcp_hdr->dst_port));
            }
            else if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
            {
                struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                *(unsigned int *)to_next[0] = ipv4_hdr->src_addr;
                *(unsigned int *)to_next[1] = ipv4_hdr->dst_addr;
                *(unsigned short *)to_next[2] = udp_hdr->src_port;
                *(unsigned short *)to_next[3] = udp_hdr->dst_port;
                // printf("task2 src: %d.%d.%d.%d:%d -> ", ipv4_hdr->src_addr & 0xFF, (ipv4_hdr->src_addr >> 8) & 0xFF, (ipv4_hdr->src_addr >> 16) & 0xFF, (ipv4_hdr->src_addr >> 24) & 0xFF, ntohs(udp_hdr->src_port));
                // printf("task2 dst: %d.%d.%d.%d:%d\n", ipv4_hdr->dst_addr & 0xFF, (ipv4_hdr->dst_addr >> 8) & 0xFF, (ipv4_hdr->dst_addr >> 16) & 0xFF, (ipv4_hdr->dst_addr >> 24) & 0xFF, ntohs(udp_hdr->dst_port));
            }
            update_sketch(*(unsigned int *)to_next[0],*(unsigned int *)to_next[1],*(unsigned short *)to_next[2],*(unsigned short *)to_next[3],res,rownum,bucketnum,bucketsize,countersize,to_next[6]);
        }
        
        return 1;
    }
    int plugin_1(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {
       if (*(unsigned int *)to_next[5] == bucketnum){
           return -1;
       }
        unsigned int min = *(int *)to_next[6];
        if (min > USER_ARG_THETA * *(unsigned int *)to_next[4])
        {
            int i=0;
            for(;i<*(unsigned int *)to_next[5];i++){
                if(*(unsigned int *)res[0][i][0] == *(unsigned int *)to_next[0]
                    && *(unsigned int *)res[0][i][1] == *(unsigned int *)to_next[1]
                    && *(unsigned short *)res[0][i][2] == *(unsigned short *)to_next[2]
                    && *(unsigned short *)res[0][i][3] == *(unsigned short *)to_next[3]){
                    *(unsigned int *)res[0][i][4] = min;
                    break;
                }
            }
            if(i == *(unsigned int *)to_next[5]){
                *(unsigned int *)res[0][*(unsigned int *)to_next[5]][0] = *(unsigned int *)to_next[0];
                *(unsigned int *)res[0][*(unsigned int *)to_next[5]][1] = *(unsigned int *)to_next[1];
                *(unsigned short *)res[0][*(unsigned int *)to_next[5]][2] = *(unsigned short *)to_next[2];
                *(unsigned short *)res[0][*(unsigned int *)to_next[5]][3] = *(unsigned short *)to_next[3];
                *(unsigned int *)res[0][*(unsigned int *)to_next[5]][4] = min;

                *(unsigned int *)to_next[5] += 1;
            }
        }
        return -1;
    }
    void query(char ****res,void *to_next[8]){
        for(int i=0;i<*(unsigned int *)to_next[5];i++){
            if(*(unsigned int *)res[0][i][4] > USER_ARG_THETA * *(unsigned int *)to_next[4]){
                printf("Heavy hitter: %u %u %u %u %u\n",*(unsigned int *)res[0][i][0],*(unsigned int *)res[0][i][1],*(unsigned short *)res[0][i][2],*(unsigned short *)res[0][i][3],*(unsigned int *)res[0][i][4]);
            }
        }
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