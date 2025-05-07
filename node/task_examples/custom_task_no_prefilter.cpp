#include <rte_ethdev.h>
//对象交集
typedef struct IntersectionObject {
    int direction;           // 0->in, 1->bi
    unsigned int src_ip;      //源地址
    unsigned int src_mask;    //源地址掩码
    unsigned int dst_ip;     //宿地址
    unsigned int dst_mask;   //宿地址掩码
    unsigned short src_port;  //源地址端口
    unsigned short dst_port; //宿地址端口
    unsigned char protocol;  //协议
} InterObject;
InterObject obj;
InterObject ** read_interobject(int *num){
    FILE *fp = fopen("/home/hjjiang/hjjiang_capture_v3/client/object.txt", "r");
    InterObject **obj = (InterObject **)malloc(32*sizeof(InterObject *));
    for(int i = 0; i < 32; i++){
        obj[i] = (InterObject *)malloc(sizeof(InterObject));
    }
    char line[256];
    int index = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        InterObject *current = obj[index];
        char src_ip_str[INET_ADDRSTRLEN];
        char src_mask_str[INET_ADDRSTRLEN];
        char dst_ip_str[INET_ADDRSTRLEN];
        char dst_mask_str[INET_ADDRSTRLEN];

        sscanf(line, "%d/%[^/]/%[^/]/%[^/]/%[^/]/%hu/%hu/%hhu",
            &current->direction,
            src_ip_str,
            src_mask_str,
            dst_ip_str,
            dst_mask_str,
            &current->src_port,
            &current->dst_port,
            &current->protocol);

        current->src_ip = ntohl(inet_addr(src_ip_str));
        current->src_mask = ntohl(inet_addr(src_mask_str));
        current->dst_ip = ntohl(inet_addr(dst_ip_str));
        current->dst_mask = ntohl(inet_addr(dst_mask_str));


        index++;
        if (index >= 32) {
            break; // Prevent buffer overflow
        }
    }
    *num = index;
    // 关闭文件
    fclose(fp);
    return obj;
}
extern "C"
{
    void init_args(void *to_next[8]){
        int i=9;
        int num;
        InterObject **objs = read_interobject(&num);
        obj.direction = objs[i]->direction;
        obj.src_ip = objs[i]->src_ip;
        obj.src_mask = objs[i]->src_mask;
        obj.dst_ip = objs[i]->dst_ip;
        obj.dst_mask = objs[i]->dst_mask;
        obj.src_port = objs[i]->src_port;
        obj.dst_port = objs[i]->dst_port;

        obj.protocol = objs[i]->protocol;
    }
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {   
        unsigned int src_ip, dst_ip;
        unsigned short src_port, dst_port;
        unsigned char protocol;
        struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
        if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV4)
        {
            struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
            
            if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
            {
                // 输出源端口和目的端口
                struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));

                src_ip = ipv4_hdr->src_addr;
                dst_ip = ipv4_hdr->dst_addr;
                src_port = ntohs(tcp_hdr->src_port);
                dst_port = ntohs(tcp_hdr->dst_port);
                protocol = IPPROTO_TCP;
            }
            else if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
            {
                // 输出源端口和目的端口
                struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                src_ip = ipv4_hdr->src_addr;
                dst_ip = ipv4_hdr->dst_addr;
                src_port = ntohs(udp_hdr->src_port);
                dst_port = ntohs(udp_hdr->dst_port);
                protocol = IPPROTO_UDP;
            }
            else
            {
                // #ifdef DEBUG
                // printf("Unsupported transport layer protocol: %d\n",ipv4_hdr->next_proto_id);
                // #endif
                return -1;
            }
        }
        else
        {
            #ifdef DEBUG
            printf("filter - Unsupported IP layer protocol: %d\n",ntohs(eth_hdr->ether_type));
            #endif
            return -1;
        }
        if(((src_ip & obj.src_mask)==obj.src_ip && (dst_ip & obj.dst_mask)==obj.dst_ip && (obj.src_port==0 || src_port==obj.src_port) && (obj.dst_port==0 || dst_port==obj.dst_port) && (obj.protocol==0 || protocol==obj.protocol)) || (obj.direction == 1 && (src_ip & obj.dst_mask)==obj.dst_ip && (dst_ip & obj.src_mask)==obj.src_ip && (obj.src_port==0 || src_port==obj.dst_port) && (obj.dst_port==0 || dst_port==obj.src_port) && (obj.protocol==0 || protocol==obj.protocol))){
            int i=1;
            int n=1;
            while(i<320){
                n=n*i%11;
                i++;
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