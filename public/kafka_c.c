#include "kafka_p_c.h"

static volatile sig_atomic_t run = 1;

/**
 * @brief Signal termination of program
 */
static void stop(int sig) {
        run = 0;
}

/**
 * @returns 1 if all bytes are printable, else 0.
 */
static int is_printable(const char *buf, size_t size) {
        size_t i;
        for (i = 0; i < size; i++)
                if (!isprint((int)buf[i]))
                        return 0;
        return 1;
}

/**
 * @brief Initialize Kafka consumer
 */
rd_kafka_t* consumer_init(const char *brokers, const char *groupid, char **topics, int topic_cnt) {
        rd_kafka_t *rk;
        rd_kafka_conf_t *conf;
        rd_kafka_resp_err_t err;
        char errstr[512];
        rd_kafka_topic_partition_list_t *subscription;
        int i;

        conf = rd_kafka_conf_new();

        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr, "%s\n", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
        }

        if (rd_kafka_conf_set(conf, "group.id", groupid, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr, "%s\n", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
        }

        if (rd_kafka_conf_set(conf, "auto.offset.reset", "earliest", errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr, "%s\n", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
        }

        rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
        if (!rk) {
                fprintf(stderr, "%% Failed to create new consumer: %s\n", errstr);
                return NULL;
        }

        rd_kafka_poll_set_consumer(rk);

        subscription = rd_kafka_topic_partition_list_new(topic_cnt);
        for (i = 0; i < topic_cnt; i++)
                rd_kafka_topic_partition_list_add(subscription, topics[i], RD_KAFKA_PARTITION_UA);

        err = rd_kafka_subscribe(rk, subscription);
        if (err) {
                fprintf(stderr, "%% Failed to subscribe to %d topics: %s\n", subscription->cnt, rd_kafka_err2str(err));
                rd_kafka_topic_partition_list_destroy(subscription);
                rd_kafka_destroy(rk);
                return NULL;
        }

        fprintf(stderr, "%% Subscribed to %d topic(s), waiting for rebalance and messages...\n", subscription->cnt);
        rd_kafka_topic_partition_list_destroy(subscription);

        return rk;
}

/**
 * @brief Consume messages from Kafka
 */
void consume_messages(rd_kafka_t *rk) {
        while (run) {
                rd_kafka_message_t *rkm = rd_kafka_consumer_poll(rk, 100);
                if (!rkm)
                        continue;

                if (rkm->err) {
                        fprintf(stderr, "%% Consumer error: %s\n", rd_kafka_message_errstr(rkm));
                        rd_kafka_message_destroy(rkm);
                        continue;
                }

                printf("Message on %s [%" PRId32 "] at offset %" PRId64 "\n",
                           rd_kafka_topic_name(rkm->rkt), rkm->partition, rkm->offset);

                if (rkm->key && is_printable((char *)rkm->key, rkm->key_len))
                        printf(" Key: %.*s\n", (int)rkm->key_len, (const char *)rkm->key);
                else if (rkm->key)
                        printf(" Key: (%d bytes)\n", (int)rkm->key_len);

                if (rkm->payload && is_printable((char *)rkm->payload, rkm->len))
                        printf(" Value: %.*s\n", (int)rkm->len, (const char *)rkm->payload);
                else if (rkm->payload)
                        printf(" Value: (%d bytes)\n", (int)rkm->len);

                rd_kafka_message_destroy(rkm);
        }
}

void consume_cleanup(rd_kafka_t *rk) {
        rd_kafka_consumer_close(rk);
        rd_kafka_destroy(rk);
}
// int main(int argc, char **argv) {
//         if (argc < 4) {
//                 fprintf(stderr, "%% Usage: %s <broker> <group.id> <topic1> <topic2>..\n", argv[0]);
//                 return 1;
//         }

//         const char *brokers = argv[1];
//         const char *groupid = argv[2];
//         char **topics = &argv[3];
//         int topic_cnt = argc - 3;

//         signal(SIGINT, stop);

//         rd_kafka_t *rk = init_consumer(brokers, groupid, topics, topic_cnt);
//         if (!rk) {
//                 return 1;
//         }

//         consume_messages(rk);

//         fprintf(stderr, "%% Closing consumer\n");
//         rd_kafka_consumer_close(rk);
//         rd_kafka_destroy(rk);

//         return 0;
// }
