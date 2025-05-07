#include "kafka_p_c.h"


static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
        if (rkmessage->err){
                fprintf(stderr, "%% Message delivery failed: %s\n", rd_kafka_err2str(rkmessage->err));
        }else{
                kafka_params *p= (kafka_params *)opaque;
                markAsFree(p->res_ring,p->dataPtr);
        }

}

rd_kafka_t* producer_init(const char *brokers, char *errstr, size_t errstr_size) {
        rd_kafka_conf_t *conf = rd_kafka_conf_new();
        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, errstr_size) != RD_KAFKA_CONF_OK) {
                fprintf(stderr, "%s\n", errstr);
                return NULL;
        }
        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
        rd_kafka_t *rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, errstr_size);
        if (!rk) {
                fprintf(stderr, "%% Failed to create new producer: %s\n", errstr);
                return NULL;
        }
        return rk;
}

void produce_message(kafka_params *k_params) {
        //构造回调函数参数
        rd_kafka_resp_err_t err;
        err = rd_kafka_producev(k_params->rk, RD_KAFKA_V_TOPIC(k_params->topic), RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY), RD_KAFKA_V_VALUE(k_params->dataPtr->data, k_params->dataPtr->size), RD_KAFKA_V_OPAQUE(k_params), RD_KAFKA_V_END);
        if (err) {
                fprintf(stderr, "%% Failed to produce to topic %s: %s\n", k_params->topic, rd_kafka_err2str(err));
                if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                        rd_kafka_poll(k_params->rk, 0);
                }
        } else {
                fprintf(stderr, "%% Enqueued message (%d bytes) for topic %s\n", k_params->dataPtr->size, k_params->topic);
        }
        rd_kafka_poll(k_params->rk, 0);
}


void produce_cleanup(rd_kafka_t *rk) {
        rd_kafka_flush(rk, 10 * 1000);
        rd_kafka_destroy(rk);
}

// typedef struct {
//         char message[512];
// } user_buf_t;

// int main(int argc, char **argv) {
//         if (argc != 3) {
//                 fprintf(stderr, "%% Usage: %s <broker> <topic>\n", argv[0]);
//                 return 1;
//         }
//         const char *brokers = argv[1];
//         const char *topic = argv[2];
//         char errstr[512];

//         _t * = producer(brokers, errstr, sizeof(errstr));
//         if (!rk) return 1;

//         user_buf_t user_buf;
//         // Assuming user_buf is filled with the message to be produced
//         // For example:
//         strcpy(user_buf.message, "This is a test message");

//         produce_message(rk, topic, user_buf.message, strlen(user_buf.message));

//         fprintf(stderr, "%% Flushing final messages..\n");
//         rd_kafka_flush(rk, 10 * 1000);
//         if (rd_kafka_outq_len(rk) > 0)
//                 fprintf(stderr, "%% %d message(s) were not delivered\n", rd_kafka_outq_len(rk));

//         rd_kafka_destroy(rk);
//         return 0;
// }
