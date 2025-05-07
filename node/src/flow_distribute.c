#include "schedule.h"
#define PORT_OFFSET 4096
#define OUTPUT 5
#define DATAPATH "128985127938"
#define IN_PORT 7

int host_to_port[MAX_HOST_NUM]={6,18,16};//only the front several ports are real.

char * construct_stat_query_args(InterObject *d){
    char match[1024];
    memset(match, 0, 1024 * sizeof(char));
    strcat(match, "\"eth_type\": 2048");
    char *m = (char *)malloc(128);
    memset(m, 0, 128 * sizeof(char));
        
    //TODO: modify when applying to real traffic
    sprintf(m,",\"in_port\": %d",IN_PORT);
    strcat(match, m);

    if(d->out_ip != 0 || d->out_mask!=0){
        
        sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
        mask_to_prefix_length(d->out_mask));
        strcat(match, m);
    }
    if(d->in_ip != 0 || d->in_mask!=0){
        sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
        mask_to_prefix_length(d->in_mask));
        strcat(match, m);
    }
    if(d->protocol != 0){
        sprintf(m, ",\"ip_proto\":%d",d->protocol);
        strcat(match, m);
    }
    char *args = (char *)malloc(1024);
    memset(args, 0, 1024 * sizeof(char));
    sprintf(args, "{\"match\": {%s}}",match);
    return args;
}

char * construct_group_args(const char *command,InterObject *d){
    int object_id = find_intersection_object(d);
    if (strcmp(command, POST_COMMAND_GROUPENTRY_ADD) == 0){
        char *args = (char *)malloc(10240);
        memset(args, 0, 10240 * sizeof(char));
        int output_num=0;
        int output[MAX_HOST_NUM];
        for(int i=0;i<MAX_HOST_NUM;i++){
            if(d->hosts[i]!=0){
                output[output_num] = i;
                output_num++;
            }
        }
        
        if(output_num==1) return NULL;
        char *buckets = (char *)malloc(10240);
        memset(buckets, 0, 10240 * sizeof(char));
        char *bucket = (char *)malloc(300);
        memset(bucket, 0, 300 * sizeof(char));
        for(int i=0;i<output_num-1;i++){
            sprintf(bucket, "{\"actions\": [{\"type\": \"OUTPUT\", \"port\": %d}]},",host_to_port[output[i]]);
            strcat(buckets, bucket);
        }
        sprintf(bucket, "{\"actions\": [{\"type\": \"OUTPUT\", \"port\": %d}]}",host_to_port[output[output_num-1]]);

        strcat(buckets, bucket);
        sprintf(args, "{\"dpid\": \"%s\",\"group_id\": %d,\"type\": \"ALL\",\"buckets\": [%s]}",
            DATAPATH,
            object_id,
            buckets
            );
        printf("args: %s\n",args);
        free(bucket);
        free(buckets);
        return args;
    }else if(strcmp(command, POST_COMMAND_GROUPENTRY_DELETE) == 0){
        char *args = (char *)malloc(1024);
        memset(args, 0, 1024 * sizeof(char));
        sprintf(args, "{\"dpid\": \"%s\",\"group_id\": %d} ",DATAPATH,object_id);
        return args;
    }else if(strcmp(command, POST_COMMAND_GROUPENTRY_MODIFY) == 0){
        char *args = (char *)malloc(10240);
        memset(args, 0, 10240 * sizeof(char));
        int output_num=0;
        int output[MAX_HOST_NUM];
        for(int i=0;i<MAX_HOST_NUM;i++){
            if(d->hosts[i]!=0){
                output[output_num] = i;
                output_num++;
            }
        }
        if(output_num==1) return NULL;
        char *buckets = (char *)malloc(10240);
        memset(buckets, 0, 10240 * sizeof(char));
        char *bucket = (char *)malloc(300);
        memset(bucket, 0, 300 * sizeof(char));
        for(int i=0;i<output_num-1;i++){
            
            sprintf(bucket, "{\"actions\": [{\"type\": \"OUTPUT\",\"port\": %d}]},",host_to_port[output[i]]);
            strcat(buckets, bucket);
        }
        sprintf(bucket, "{\"actions\": [{\"type\": \"OUTPUT\", \"port\": %d}]}",host_to_port[output[output_num-1]]);
        strcat(buckets, bucket);
        sprintf(args, "{\"dpid\": \"%s\",\"group_id\": %d,\"type\": \"ALL\",\"buckets\": [%s]}",
            DATAPATH,
            object_id,
            buckets
            );
        free(bucket);
        free(buckets);
        return args;
    }else{
        return NULL;
    }
}
int construct_flow_args(const char *command,InterObject *d,char *flows[MAX_FLOW_NUM_PER_OBJECT]){
    int object_id = find_intersection_object(d);
    int num = 0;
    int output_num=0;
    int output[MAX_HOST_NUM];
    for(int i=0;i<MAX_HOST_NUM;i++){
        if(d->hosts[i]!=0){
            output[output_num] = i;
            output_num++;
        }
    }
    if (strcmp(command, POST_COMMAND_FLOWENTRY_ADD) == 0){
        if(output_num == 1){// 无需分流
            if(d->direction == 0){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip >>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                #ifdef test
                printf("args: %s\n",args);
                #endif
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else if(d->direction == 1){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                #ifdef test
                printf("args: %s\n",args);
                #endif
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else{
                char *args1 = (char *)malloc(1024);
                memset(args1, 0, 1024 * sizeof(char));
                char match1[1024];
                memset(match1, 0, 1024 * sizeof(char));
                strcat(match1, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match1, m);


                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match1, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match1, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match1, m);
                }
                sprintf(args1, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match1,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                #ifdef test
                printf("args: %s\n",args1);
                #endif
                num ++;
                flows[num-1] = args1;

                char *args2 = (char *)malloc(1024);
                memset(args2, 0, 1024 * sizeof(char));
                char match2[1024];
                memset(match2, 0, 1024 * sizeof(char));
                strcat(match2, "\"eth_type\": 2048");

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match2, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match2, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match2, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match2, m);
                }
                sprintf(args2, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match2,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                #ifdef test
                printf("args: %s\n",args2);
                #endif
                num ++;
                flows[num-1] = args2;
                free(m);
                return num;
            }
        }else{
            if(d->direction == 0){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                #ifdef test
                printf("args: %s\n",args);
                #endif
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else if(d->direction == 1){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);
                
                if(d->in_ip != 0 || d->in_mask!=0){
                    
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                #ifdef test
                printf("args: %s\n",args);
                #endif
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else{
                char *args1 = (char *)malloc(1024);
                memset(args1, 0, 1024 * sizeof(char));
                char match1[1024]="";
                strcat(match1, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match1, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match1, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match1, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match1, m);
                }
                sprintf(args1, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match1,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                #ifdef test
                printf("args: %s\n",args1);
                #endif
                num ++;
                flows[num-1] = args1;

                char *args2 = (char *)malloc(1024);
                memset(args2, 0, 1024 * sizeof(char));
                char match2[1024];
                memset(match2, 0, 1024 * sizeof(char));
                strcat(match2, "\"eth_type\": 2048");

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match2, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match2, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match2, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match2, m);
                }
                sprintf(args2, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match2,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                #ifdef test
                printf("args: %s\n",args2);
                #endif
                num ++;
                flows[num-1] = args2;
                free(m);
                return num;
            }
        }
        
    }else if(strcmp(command, POST_COMMAND_FLOWENTRY_DELETE) == 0){
        if(d->direction == 0){
            char *args = (char *)malloc(1024);
            memset(args, 0, 1024 * sizeof(char));
            char match[1024];
            memset(match, 0, 1024 * sizeof(char));
            strcat(match, "\"eth_type\": 2048");
            char *m = (char *)malloc(128);
            memset(m, 0, 128 * sizeof(char));

            //TODO: modify when applying to real traffic
            sprintf(m,",\"in_port\": %d",IN_PORT);
            strcat(match, m);

            if(d->out_ip != 0 || d->out_mask!=0){
                
                sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                mask_to_prefix_length(d->out_mask));
                strcat(match, m);
            }
            if(d->in_ip != 0 || d->in_mask!=0){
                sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                mask_to_prefix_length(d->in_mask));
                strcat(match, m);
                // free(m);
            }
            if(d->protocol != 0){
                sprintf(m, ",\"ip_proto\":%d",d->protocol);
                strcat(match, m);
            }
            sprintf(args, "{\"dpid\": \"%s\",\"match\": {%s}}",
                DATAPATH,
                match
                );
            #ifdef test
                printf("args: %s\n",args);
                #endif
            num ++;
            flows[num-1] = args;
            free(m);
            return num;
        }else if(d->direction == 1){
            char *args = (char *)malloc(1024);
            memset(args, 0, 1024 * sizeof(char));
            char match[1024];
            memset(match, 0, 1024 * sizeof(char));
            strcat(match, "\"eth_type\": 2048");
            char *m = (char *)malloc(128);
            memset(m, 0, 128 * sizeof(char));
            
            //TODO: modify when applying to real traffic
            sprintf(m,",\"in_port\": %d",IN_PORT);
            strcat(match, m);

            if(d->in_ip != 0 || d->in_mask!=0){
                
                sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                mask_to_prefix_length(d->in_mask));
                strcat(match, m);
                // free(m);
            }
            if(d->out_ip != 0 || d->out_mask!=0){
                sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                mask_to_prefix_length(d->out_mask));
                strcat(match, m);
            }
            if(d->protocol != 0){
                sprintf(m, ",\"ip_proto\":%d",d->protocol);
                strcat(match, m);
            }
            sprintf(args, "{\"dpid\": \"%s\",\"match\": {%s}}",
                DATAPATH,
                match
                );
            #ifdef test
            printf("args: %s\n",args);
            #endif
            num ++;
            flows[num-1] = args;
            free(m);
            return num;
        }else if(d->direction == 2){
            char *args1 = (char *)malloc(1024);
            memset(args1, 0, 1024 * sizeof(char));
            char match1[1024]="";
            strcat(match1, "\"eth_type\": 2048");
            char *m = (char *)malloc(128);
            memset(m, 0, 128 * sizeof(char));
            
            //TODO: modify when applying to real traffic
            sprintf(m,",\"in_port\": %d",IN_PORT);
            strcat(match1, m);

            if(d->in_ip != 0 || d->in_mask!=0){
                sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                mask_to_prefix_length(d->in_mask));
                strcat(match1, m);
                // free(m);
            }
            if(d->out_ip != 0 || d->out_mask!=0){
                sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                mask_to_prefix_length(d->out_mask));
                strcat(match1, m);
            }
            if(d->protocol != 0){
                sprintf(m, ",\"ip_proto\":%d",d->protocol);
                strcat(match1, m);
            }
            sprintf(args1, "{\"dpid\": \"%s\",\"match\": {%s}}",
                DATAPATH,
                match1
                );
            #ifdef test
            printf("args: %s\n",args1);
            #endif
            flows[num] = args1;
            num ++;
            

            char *args2 = (char *)malloc(1024);
            memset(args2, 0, 1024 * sizeof(char));
            char match2[1024];
            memset(match2, 0, 1024 * sizeof(char));
            strcat(match2, "\"eth_type\": 2048");
            
            //TODO: modify when applying to real traffic
            sprintf(m,",\"in_port\": %d",IN_PORT);
            strcat(match2, m);

            if(d->out_ip != 0 || d->out_mask!=0){
                sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                mask_to_prefix_length(d->out_mask));
                strcat(match2, m);
            }
            if(d->in_ip != 0 || d->in_mask!=0){
                sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                mask_to_prefix_length(d->in_mask));
                strcat(match2, m);
                // free(m);
            }
            if(d->protocol != 0){
                sprintf(m, ",\"ip_proto\":%d",d->protocol);
                strcat(match2, m);
            }
            sprintf(args2, "{\"dpid\": \"%s\",\"match\": {%s}}",
                DATAPATH,
                match2
                );
            flows[num] = args2;
            #ifdef test
            printf("args: %s\n",args2);
            #endif
            num ++;
            free(m);
            return num;
        }
    }else if(strcmp(command, POST_COMMAND_FLOWENTRY_MODIFY) == 0){
       if(output_num == 1){// 无需分流
            if(d->direction == 0){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));

                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else if(d->direction == 1){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else{
                char *args1 = (char *)malloc(1024);
                memset(args1, 0, 1024 * sizeof(char));
                char match1[1024]="";
                strcat(match1, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match1, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match1, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match1, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match1, m);
                }
                sprintf(args1, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match1,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                num ++;
                flows[num-1] = args1;

                char *args2 = (char *)malloc(1024);
                memset(args2, 0, 1024 * sizeof(char));
                char match2[1024];
                memset(match2, 0, 1024 * sizeof(char));
                strcat(match2, "\"eth_type\": 2048");
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match2, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match2, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match2, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match2, m);
                }
                sprintf(args2, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"OUTPUT\", \"port\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match2,
                    object_id+PORT_OFFSET,
                    host_to_port[output[0]]
                    );
                num ++;
                flows[num-1] = args2;
                free(m);
                return num;
            }
        }else{
            if(d->direction == 0){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else if(d->direction == 1){
                char *args = (char *)malloc(1024);
                memset(args, 0, 1024 * sizeof(char));
                char match[1024];
                memset(match, 0, 1024 * sizeof(char));
                strcat(match, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match, m);
                }
                sprintf(args, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                num ++;
                flows[num-1] = args;
                free(m);
                return num;
            }else{
                char *args1 = (char *)malloc(1024);
                memset(args1, 0, 1024 * sizeof(char));
                char match1[1024]="";
                strcat(match1, "\"eth_type\": 2048");
                char *m = (char *)malloc(128);
                memset(m, 0, 128 * sizeof(char));
                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match1, m);

                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match1, m);
                    // free(m);
                }
                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match1, m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match1, m);
                }
                sprintf(args1, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match1,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                num ++;
                flows[num-1] = args1;

                char *args2 = (char *)malloc(1024);
                memset(args2, 0, 1024 * sizeof(char));
                char match2[1024];
                memset(match2, 0, 1024 * sizeof(char));
                strcat(match2, "\"eth_type\": 2048");
                                
                //TODO: modify when applying to real traffic
                sprintf(m,",\"in_port\": %d",IN_PORT);
                strcat(match2, m);

                if(d->out_ip != 0 || d->out_mask!=0){
                    sprintf(m, ",\"ipv4_src\":\"%d.%d.%d.%d/%d\"",(d->out_ip>>24)&0xff,(d->out_ip>>16)&0xff,(d->out_ip>>8)&0xff,d->out_ip&0xff,
                    mask_to_prefix_length(d->out_mask));
                    strcat(match2, m);
                }
                if(d->in_ip != 0 || d->in_mask!=0){
                    sprintf(m, ",\"ipv4_dst\":\"%d.%d.%d.%d/%d\"",(d->in_ip>>24)&0xff,(d->in_ip>>16)&0xff,(d->in_ip>>8)&0xff,d->in_ip&0xff,
                    mask_to_prefix_length(d->in_mask));
                    strcat(match2, m);
                    // free(m);
                }
                if(d->protocol != 0){
                    sprintf(m, ",\"ip_proto\":%d",d->protocol);
                    strcat(match2, m);
                }
                sprintf(args2, "{\"dpid\": \"%s\",\"priority\": %d,\"match\": {%s},\"actions\": [{\"type\": \"PUSH_VLAN\",\"ethertype\": 33024},{\"type\": \"SET_FIELD\",\"field\":\"vlan_vid\",\"value\": %d},{\"type\": \"GROUP\", \"group_id\": %d}]}",
                    DATAPATH,
                    d->priority,
                    match2,
                    object_id+PORT_OFFSET,
                    object_id
                    );
                num ++;
                flows[num-1] = args2;
                free(m);
                return num;
            }
        }
        
    }
    return 0;
    
}

bool schedule_object_to_host(InterObject *d){
    int output_num=0;
    // int output[MAX_HOST_NUM];
    for(int i=0;i<MAX_HOST_NUM;i++){
        if(d->hosts[i]!=0){
            // output[output_num] = i;
            output_num++;
        }
    }
    if(output_num==1){ // 不需要分流
        if(d->is_new){
            char *flows[MAX_FLOW_NUM_PER_OBJECT];
            int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_ADD,d,flows);
            for(int i=0;i<flow_num;i++){
                printf("-->add flow: %s\n",flows[i]);
                post_command_without_result(POST_COMMAND_FLOWENTRY_ADD,flows[i]);
            }
        }else{
            char *flows[MAX_FLOW_NUM_PER_OBJECT];
            int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_MODIFY,d,flows);
            for(int i=0;i<flow_num;i++){
                printf("-->mod flow: %s\n",flows[i]);
                post_command_without_result(POST_COMMAND_FLOWENTRY_MODIFY,flows[i]);
            }
        }
    }else{ // 需要分流
        if(d->is_new){
            char *group_args = construct_group_args(POST_COMMAND_GROUPENTRY_ADD,d);
            post_command_without_result(POST_COMMAND_GROUPENTRY_ADD,group_args);
            char *flows[MAX_FLOW_NUM_PER_OBJECT];
            int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_ADD,d,flows);
            for(int i=0;i<flow_num;i++){
                printf("-->add flow: %s\n",flows[i]);
                post_command_without_result(POST_COMMAND_FLOWENTRY_ADD,flows[i]);
            }
        }else{
            char *group_args = construct_group_args(POST_COMMAND_FLOWENTRY_MODIFY,d);
            post_command_without_result(POST_COMMAND_FLOWENTRY_MODIFY,group_args);
            char *flows[MAX_FLOW_NUM_PER_OBJECT];
            int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_MODIFY,d,flows);
            for(int i=0;i<flow_num;i++){
                printf("-->mod flow: %s\n",flows[i]);
                post_command_without_result(POST_COMMAND_FLOWENTRY_MODIFY,flows[i]);
            }
        }
    }
    return true;
}

bool remove_object_from_host(InterObject *d){
    int output_num=0;
    for(int i=0;i<MAX_HOST_NUM;i++){
        if(d->hosts[i]!=0){
            output_num++;
        }
    }
    if(output_num==1){ // 不需要分流
        char *flows[MAX_FLOW_NUM_PER_OBJECT];
        int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_DELETE,d,flows);
        for(int i=0;i<flow_num;i++){
            post_command_without_result(POST_COMMAND_FLOWENTRY_DELETE,flows[i]);
        }
    }else{ // 需要分流
        char *flows[MAX_FLOW_NUM_PER_OBJECT];
        int flow_num = construct_flow_args(POST_COMMAND_FLOWENTRY_DELETE,d,flows);
        for(int i=0;i<flow_num;i++){
            post_command_without_result(POST_COMMAND_FLOWENTRY_DELETE,flows[i]);
        }
        char *group_args = construct_group_args(POST_COMMAND_GROUPENTRY_DELETE,d);
        post_command_without_result(POST_COMMAND_GROUPENTRY_DELETE,group_args);
    }
    return true;
}