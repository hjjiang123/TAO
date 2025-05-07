#ifndef STATISTICS_H
#define STATISTICS_H
#include <stdint.h>

struct loss_statistic {
    int thread_id;
    int actor_id;
    int global_actor_id;
    int loss[32];
};

struct pipeline_statistics {
    uint32_t read_packets_per_duration;
    uint64_t cycles_cost_per_duration;
    // uint64_t cycles_cost_dispatch_per_duration;
    uint32_t read_packets;
    int num_0; //取得0个报文的次数
    int num_1; //取得1个报文的次数
    int num_2; //取得1个及以上报文的次数
    int loss; //丢包数
    int actor_loss[MAX_MULTI_PIPELINE_NUM];

    int max_ring_len;

    //filter统计字段
    int filter_num;
    int match_count;
    int pkt_count;
    int total_match_count;
    int total_pkt_count;
};
#endif