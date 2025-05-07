#include "publish_middlebox.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <librdkafka/rdkafka.h>

typedef struct {
    ResultDataRing *res_ring;
    int res_ring_size;
    ResultData *dataPtr;
    rd_kafka_t* rk;
    char topic[NAME_MAX];
} kafka_params;

// producer
rd_kafka_t* producer_init(const char *brokers, char *errstr, size_t errstr_size);
void produce_message(kafka_params *k_params);
void produce_cleanup(rd_kafka_t *rk);

// consumer
rd_kafka_t* consumer_init(const char *brokers, const char *groupid, char **topics, int topic_cnt);
void consume_messages(rd_kafka_t *rk);
void consume_cleanup(rd_kafka_t *rk);