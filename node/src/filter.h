#ifndef FILTER_H
#define FILTER_H
#include "object.h"
#include "param.h"
typedef struct filter {
    InterObject obj;
    int filter_id;

    bool hit_exit;
    int hit_to;
    bool miss_exit;
    int miss_to;
} Filter;

struct tree_node {
    int filter_id;
    int priority;
    int child_count;
    int children[16];
    int i; //记录遍历到哪个子节点了
    bool is_leaf;
};


typedef struct {
    Filter filters[MAX_INTERSECTION_NUM];
    int count;
    int pkt_count;
    int match_count;
    int total_pkt_count;
    int total_match_count;
    int head;
} FilterGroup;

// void initial_filter();
// Function to initialize the filter group
FilterGroup *initializeFilterGroup();

// Function to compare filters by priority for sorting
int compareByPriority(const void *a, const void *b) ;

// Function to sort filters by priority
void sortFiltersByPriority(FilterGroup *group) ;
FilterGroup * create_filter_group();
void free_filter_group(FilterGroup *group);
// Function to match filters against incoming packet details
unsigned int matchFilter(FilterGroup *group, struct rte_mbuf *pkt) ;
#endif
