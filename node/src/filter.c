#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rte_lcore.h>
#include <rte_ethdev.h>
#include "filter.h"

// int cur_count[THREAD_INPUT_THREAD_NUM];
// int total_count[THREAD_INPUT_THREAD_NUM];
// void initial_filter(){
//     for(int i=0;i<THREAD_INPUT_THREAD_NUM;i++){
//         cur_count[i] = 0;
//         total_count[i] = 0;
//     }
// }
// void get_count(int *cur,int *total){
//     for(int i=0;i<THREAD_INPUT_THREAD_NUM;i++){
//         cur[i] = cur_count[i];
//         total[i] = total_count[i];
//     }
// }

// Function to initialize the filter group
FilterGroup *initializeFilterGroup() {
    
    FilterGroup *group = (FilterGroup *)malloc(sizeof(FilterGroup));
    group->count = 0;
    group->pkt_count = 0;
    group->match_count = 0;
    group->total_pkt_count = 0;
    group->total_match_count = 0;
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        InterObject *obj = intersection_object(i);
        if(obj){
            group->filters[group->count].obj = *obj;
            group->filters[group->count].filter_id = i;
            group->count++;
            #ifdef DEBUG_FILTER_1
            printf("filter - Filter %d: src_ip: %u, src_mask: %u, dst_ip: %u, dst_mask: %u, src_port: %u, dst_port: %u, protocol: %u, priority: %u, direction: %u,task_id:", i, obj->src_ip, obj->src_mask, obj->dst_ip, obj->dst_mask, obj->src_port, obj->dst_port, obj->protocol, obj->priority, obj->direction);
            for(int j=0;j<obj->priority;j++){
                printf(" %d", obj->task_ids[j]);
            }
            printf("\n");
            #endif
        }
    }
    return group;
}

// Function to compare filters by priority for sorting
int compareByPriority(const void *a, const void *b) {
    Filter *filterA = (Filter *)a;
    Filter *filterB = (Filter *)b;
    return filterA->obj.priority - filterB->obj.priority;
}

// Function to sort filters by priority
void sortFiltersByPriority(FilterGroup *group) {
    qsort(group->filters, group->count, sizeof(Filter), compareByPriority);
}

bool is_child_filter(FilterGroup *group,int parent_node,int child_node){
    InterObject *parent_obj = &group->filters[parent_node].obj;
    InterObject *child_obj = &group->filters[child_node].obj;
    if(contain_intersection_object_or_not(parent_obj,child_obj)){
        return true;
    }
    return false;
}

int find_child_filters(FilterGroup *group,struct tree_node *node, int priority, int root_next_child[16],int *root_next_child_count, int induces[MAX_INTERSECTION_NUM]){
    *root_next_child_count = 0;
    int min_over_priority = 32;
    if(node->filter_id == -1){
        for (int i = 0; i < group->count; i++) {
            InterObject *obj = &group->filters[i].obj;
            if(obj->priority > priority && obj->priority < min_over_priority){
                min_over_priority = obj->priority;
                *root_next_child_count = 1;
                root_next_child[0] = i;
            }else if(obj->priority >priority && obj->priority == min_over_priority){
                root_next_child[(*root_next_child_count)++] = i;
            }
        }
    }else{
        InterObject *current_obj = &group->filters[induces[node->filter_id]].obj;
        for (int i = 0; i < group->count; i++) {
            InterObject *obj = &group->filters[i].obj;
            if(contain_intersection_object_or_not(current_obj,obj)){
                if(obj->priority > priority && obj->priority < min_over_priority){
                    min_over_priority = obj->priority;
                    *root_next_child_count = 1;
                    root_next_child[0] = i;
                }else if(obj->priority >priority && obj->priority == min_over_priority){
                    root_next_child[(*root_next_child_count)++] = i;
                }
            }
        }
    }
    
    return min_over_priority;
}

void recursive_build_tree(FilterGroup *group,struct tree_node *tree,int current_node, int current_priority, int induces[MAX_INTERSECTION_NUM], bool filter_is_build[]){
    int root_next_child[16];
    int root_next_child_count;
    int min_over_priority = find_child_filters(group,&tree[current_node],current_priority,root_next_child,&root_next_child_count,induces);
    if(root_next_child_count == 0){
        tree[current_node].is_leaf = true;
        return;
    }

    for(int i=0;i<root_next_child_count;i++){
        if(filter_is_build[root_next_child[i]]){
            continue;
        }
        tree[current_node].children[tree[current_node].child_count++] = root_next_child[i];
        tree[root_next_child[i]].priority = min_over_priority;
        
        // group->filters[current_node].hit_to = root_next_child[i];
        // group->filters[root_next_child[i]].miss_output = group->filters[current_node].filter_id;

        filter_is_build[root_next_child[i]] = true;

        #ifdef DEBUG_FILTER_1
        printf("filter - node:%d child: %d\n",current_node,root_next_child[i]);
        #endif
        
        recursive_build_tree(group,tree,root_next_child[i],min_over_priority,induces,filter_is_build);
    }
}

void recursive_build_group_matchpath(FilterGroup *group,struct tree_node *tree,int current_node){
    struct tree_node *current = &tree[current_node];
    if(current->child_count >1){
        group->filters[current_node].hit_exit = false;
        group->filters[current_node].hit_to = current->children[0];
        for(int i=0;i<current->child_count-1;i++){
            group->filters[current->children[i]].miss_exit = false;
            group->filters[current->children[i]].miss_to = current->children[i+1];
        }
        group->filters[current->children[current->child_count-1]].miss_exit = true;
        group->filters[current->children[current->child_count-1]].miss_to = current->filter_id;
    }else if(current->child_count == 1){
        group->filters[current_node].hit_exit = false;
        group->filters[current_node].hit_to = current->children[0];
        group->filters[current->children[0]].miss_exit = true;
        group->filters[current->children[0]].miss_to = current->filter_id; 
    }else{
        group->filters[current_node].hit_exit = true;
        group->filters[current_node].hit_to = current->filter_id;
    }
    for(int i=0;i<current->child_count;i++){
        recursive_build_group_matchpath(group,tree,current->children[i]);
    }

}

void print_tree(struct tree_node *tree,int current_node){
    printf("filter - tree node: %d\n",tree[current_node].filter_id);
    for(int i=0;i<tree[current_node].child_count;i++){
        print_tree(tree,tree[current_node].children[i]);
    }
}
void free_filter_group(FilterGroup *group){
    free(group);
}
FilterGroup * create_filter_group(){
    FilterGroup *group  = initializeFilterGroup();
    sortFiltersByPriority(group);
    int induces[MAX_INTERSECTION_NUM]; //记录每个对象交集在过滤器组中的位置
    for(int i=0;i<group->count;i++){
        induces[group->filters[i].filter_id] = i;
    }
    #ifdef DEBUG_FILTER_1
    printf("filter - induces: ");
    for(int i=0;i<group->count;i++){
        printf("%d ",induces[i]);
    }
    printf("\n");
    #endif

    // 构造过滤器树的数组
    struct tree_node *tree = (struct tree_node *)malloc(sizeof(struct tree_node)*(group->count+1));
    for(int i=0;i<group->count;i++){
        tree[i].filter_id = group->filters[i].filter_id;
        tree[i].priority = group->filters[i].obj.priority;
        tree[i].child_count = 0;
        tree[i].i = 0;
        tree[i].is_leaf = false;
    }
    // 构建起始节点
    struct tree_node *root = &tree[group->count];
    root->filter_id = -1;
    root->priority = 0;
    root->child_count = 0;
    root->i = 0;
    root->is_leaf = false;
    
    // 构建过滤器树
    bool filter_is_build[MAX_INTERSECTION_NUM];
    for(int i=0;i<group->count;i++){
        filter_is_build[i] = false;
    }
    recursive_build_tree(group,tree,group->count,0,induces,filter_is_build);
    #ifdef DEBUG_FILTER_1
    print_tree(tree,group->count);
    #endif
    
    // 基于tree在group中构建匹配路径
    recursive_build_group_matchpath(group,tree,group->count);
    group->head = group->filters[group->count].hit_to;

    free(tree);

    #ifdef DEBUG_FILTER_1
    for(int i=0;i<group->count;i++){
        printf("filter - Filter %d: filter_id:%d, hit_exit: %d, hit_to: %d, miss_exit: %d, miss_to: %d\n", 
            i,group->filters[i].filter_id, group->filters[i].hit_exit, group->filters[i].hit_to, 
            group->filters[i].miss_exit, group->filters[i].miss_to);
    }
    #endif
    return group;
}



// Function to match filters against incoming packet details
unsigned int matchFilter(FilterGroup *group, struct rte_mbuf *pkt) {
    if(!group){
        return 65535;
    }
    unsigned int src_ip, dst_ip;
    unsigned short src_port, dst_port; 
    unsigned char protocol;
    struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
    #ifdef DEBUG_REAL_TRAFFIC
    struct rte_vlan_hdr *vlan_hdr;
    if (ntohs(eth_hdr->ether_type) == RTE_ETHER_TYPE_VLAN)
    {
        vlan_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_vlan_hdr *, sizeof(struct rte_ether_hdr));
    }
    if (ntohs(vlan_hdr->eth_proto) == RTE_ETHER_TYPE_IPV4){
        struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, (sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr)));
        if (ipv4_hdr->next_proto_id == IPPROTO_TCP)
        {
            // 输出源端口和目的端口
            struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr) + sizeof(struct rte_ipv4_hdr));

            src_ip = ipv4_hdr->src_addr;
            dst_ip = ipv4_hdr->dst_addr;
            src_port = ntohs(tcp_hdr->src_port);
            dst_port = ntohs(tcp_hdr->dst_port);
            protocol = IPPROTO_TCP;
        }
        else if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
        {
            // 输出源端口和目的端口
            struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr) + sizeof(struct rte_ipv4_hdr));
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
            return 65535;
        }
    }
    #else
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
            return 65535;
        }
    }
    #endif
    else
    {
        #ifdef DEBUG
        printf("filter - Unsupported IP layer protocol: %d\n",ntohs(eth_hdr->ether_type));
        #endif
        return 65535;
    }
    
    group->pkt_count++;
    Filter *filter = &group->filters[group->head];
    bool is_hit = false;
    while(true){
        group->match_count++;
        InterObject *obj = &filter->obj;
        if(((src_ip & obj->src_mask)==obj->src_ip && (dst_ip & obj->dst_mask)==obj->dst_ip && (obj->src_port==0 || src_port==obj->src_port) && (obj->dst_port==0 || dst_port==obj->dst_port) && (obj->protocol==0 || protocol==obj->protocol))  ||  (filter->obj.direction == 1 && (src_ip & obj->dst_mask)==obj->dst_ip && (dst_ip & obj->src_mask)==obj->src_ip && (obj->src_port==0 || src_port==obj->dst_port) && (obj->dst_port==0 || dst_port==obj->src_port) && (obj->protocol==0 || protocol==obj->protocol))){
            if(filter->hit_exit){
                return filter->hit_to;
            }else{
                filter = &group->filters[filter->hit_to];
            }
        }else{
            if(filter->miss_exit){
                return filter->miss_to;
            }else{
                filter = &group->filters[filter->miss_to];
            }
        }
    }
    return 65535;
}

