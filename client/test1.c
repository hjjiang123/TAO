#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "object.h"
#include "param.h"
#include "command.h"

InterObject ** read_interobject(int *num){
    FILE *fp = fopen("object.txt", "r");
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

        for (int i = 0; i < MAX_HOST_NUM; i++) {
            current->hosts[i] = 0; // Initialize hosts array to 0
        }

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
int main(){

    // 1.all traffic
    InterObject *obj0 = (InterObject *)malloc(sizeof(InterObject));
    obj0->direction = 1;
    obj0->src_ip = 0;
    obj0->src_mask = 0;
    obj0->dst_ip = 0;
    obj0->dst_mask = 0;
    obj0->src_port = 0;
    obj0->dst_port = 0;
    obj0->protocol = 0;
    obj0->priority = 0;
    obj0->parrent_obj_id = -1;
    obj0->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj0->hosts[i] = 0;
    }

    // 2.逐渐缩小
    InterObject *obj110 = (InterObject *)malloc(sizeof(InterObject));
    obj110->direction = 0;
    obj110->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj110->src_mask = ntohl(inet_addr("255.255.255.128"));
    obj110->dst_ip = 0;
    obj110->dst_mask = 0;
    obj110->src_port = 0;
    obj110->dst_port = 0;
    obj110->protocol = 0;
    obj110->priority = 0;
    obj110->parrent_obj_id = -1;
    obj110->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj110->hosts[i] = 0;
    }
   
    InterObject *obj111 = (InterObject *)malloc(sizeof(InterObject));
    obj111->direction = 0;
    obj111->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj111->src_mask = ntohl(inet_addr("255.255.255.128"));
    obj111->dst_ip = 0;
    obj111->dst_mask = 0;
    obj111->src_port = 0;
    obj111->dst_port = 0;
    obj111->protocol = 0;
    obj111->priority = 0;
    obj111->parrent_obj_id = -1;
    obj111->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj111->hosts[i] = 0;
    }

    InterObject *obj120 = (InterObject *)malloc(sizeof(InterObject));
    obj120->direction = 0;
    obj120->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj120->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj120->dst_ip = 0;
    obj120->dst_mask = 0;
    obj120->src_port = 0;
    obj120->dst_port = 0;
    obj120->protocol = 0;
    obj120->priority = 0;
    obj120->parrent_obj_id = -1;
    obj120->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj120->hosts[i] = 0;
    }

    InterObject *obj121 = (InterObject *)malloc(sizeof(InterObject));
    obj121->direction = 0;
    obj121->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj121->src_mask = ntohl(inet_addr("255.255.255.192"));
    obj121->dst_ip = 0;
    obj121->dst_mask = 0;
    obj121->src_port = 0;
    obj121->dst_port = 0;
    obj121->protocol = 0;
    obj121->priority = 0;
    obj121->parrent_obj_id = -1;
    obj121->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj121->hosts[i] = 0;
    }

    InterObject *obj130 = (InterObject *)malloc(sizeof(InterObject));
    obj130->direction = 0;
    obj130->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj130->src_mask = ntohl(inet_addr("255.255.255.224"));
    obj130->dst_ip = 0;
    obj130->dst_mask = 0;
    obj130->src_port = 0;
    obj130->dst_port = 0;
    obj130->protocol = 0;
    obj130->priority = 0;
    obj130->parrent_obj_id = -1;
    obj130->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj130->hosts[i] = 0;
    }

    InterObject *obj131 = (InterObject *)malloc(sizeof(InterObject));
    obj131->direction = 0;
    obj131->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj131->src_mask = ntohl(inet_addr("255.255.255.224"));
    obj131->dst_ip = 0;
    obj131->dst_mask = 0;
    obj131->src_port = 0;
    obj131->dst_port = 0;
    obj131->protocol = 0;
    obj131->priority = 0;
    obj131->parrent_obj_id = -1;
    obj131->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj131->hosts[i] = 0;
    }

    InterObject *obj140 = (InterObject *)malloc(sizeof(InterObject));
    obj140->direction = 0;
    obj140->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj140->src_mask = ntohl(inet_addr("255.255.255.240"));
    obj140->dst_ip = 0;
    obj140->dst_mask = 0;
    obj140->src_port = 0;
    obj140->dst_port = 0;
    obj140->protocol = 0;
    obj140->priority = 0;
    obj140->parrent_obj_id = -1;
    obj140->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj140->hosts[i] = 0;
    }

    InterObject *obj141 = (InterObject *)malloc(sizeof(InterObject));
    obj141->direction = 1;
    obj141->src_ip = ntohl(inet_addr("1.0.0.0"));;
    obj141->src_mask = ntohl(inet_addr("255.255.255.240"));
    obj141->dst_ip = 0;
    obj141->dst_mask = 0;
    obj141->src_port = 0;
    obj141->dst_port = 0;
    obj141->protocol = 0;
    obj141->priority = 0;
    obj141->parrent_obj_id = -1;
    obj141->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj141->hosts[i] = 0;
    }

    
    InterObject *obj22 = (InterObject *)malloc(sizeof(InterObject));
    obj22->direction = 1;
    obj22->src_ip = ntohl(inet_addr("0.0.0.0"));
    obj22->src_mask = ntohl(inet_addr("224.0.0.0"));
    obj22->dst_ip = 0;
    obj22->dst_mask = 0;
    obj22->src_port = 0;
    obj22->dst_port = 0;
    obj22->protocol = 0;
    obj22->priority = 0;
    obj22->parrent_obj_id = -1;
    obj22->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj22->hosts[i] = 0;
    }


    InterObject *obj23 = (InterObject *)malloc(sizeof(InterObject));
    obj23->direction = 1;
    obj23->src_ip = ntohl(inet_addr("0.0.0.0"));
    obj23->src_mask = ntohl(inet_addr("128.0.0.0"));
    obj23->dst_ip = ntohl(inet_addr("0.0.0.0"));
    obj23->dst_mask = ntohl(inet_addr("192.0.0.0"));
    obj23->src_port = 0;
    obj23->dst_port = 0;
    obj23->protocol = 0;
    obj23->priority = 0;
    obj23->parrent_obj_id = -1;
    obj23->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj23->hosts[i] = 0;
    }

    InterObject *obj24 = (InterObject *)malloc(sizeof(InterObject));
    obj24->direction = 1;
    obj24->src_ip = ntohl(inet_addr("0.0.0.0"));
    obj24->src_mask = ntohl(inet_addr("192.0.0.0"));
    obj24->dst_ip = ntohl(inet_addr("0.0.0.0"));
    obj24->dst_mask = ntohl(inet_addr("128.0.0.0"));
    obj24->src_port = 0;
    obj24->dst_port = 0;
    obj24->protocol = 0;
    obj24->priority = 0;
    obj24->parrent_obj_id = -1;
    obj24->is_new = false;
    for(int i = 0; i < MAX_HOST_NUM; i++){
        obj24->hosts[i] = 0;
    }

    #ifdef TEST1
    //task0 - baseline
    struct subtask_params tp0;
    strcpy(tp0.name, "normal_TEST1_baseline");
    tp0.obj_num = 1;
    tp0.objs[0] = *obj0;
    strcpy(tp0.filename, "baseline.so");
    tp0.pi_num = 1;
    struct plugin_params p_params01 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "baseline.so",
        .funcname = "plugin_0"
    };
    tp0.pis[0] = p_params01;
    tp0.time = 1800;
    tp0.epoch = 60;
    tp0.cpu_num = 2;
    tp0.mem_num = 1*1*2*4/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;
    struct command_req req;
    struct command_rsp rsp;
    req.type = TASK_SUBMIT;
    req.task_submit = tp0;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 0);
        return false;
    }
    sleep(1);

    //task1 - print packet info
    struct subtask_params tp1;
    strcpy(tp1.name, "normal_TEST1_task1");
    tp1.obj_num = 1;
    tp1.objs[0] = *obj0;

    strcpy(tp1.filename, "task1.so");
    tp1.pi_num = 1;
    struct plugin_params p_params11 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "task1.so",
        .funcname = "plugin_0"
    };
    tp1.pis[0] = p_params11;
    tp1.time = 1800;
    tp1.epoch = 60;
    tp1.cpu_num = 1;
    tp1.mem_num = 1*1*2*4/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;

    req.task_submit = tp1;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 1);
        return false;
    }
    sleep(1);

    //task2 - heavy hitter detection
    struct subtask_params tp2;
    strcpy(tp2.name, "normal_TEST1_task2");
    tp2.obj_num = 1;
    tp2.objs[0] = *obj0;
    strcpy(tp2.filename, "task2.so");
    tp2.pi_num = 2;
    struct plugin_params p_params21 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_0"
    };
    tp2.pis[0] = p_params21;
    struct plugin_params p_params22 = {
        .plugin_index = 1,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 128,
            .bucketsize = 5,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_1"
    };
    tp2.pis[1] = p_params22;
    tp2.time = 1800;
    tp2.epoch = 60;
    tp2.cpu_num = 1;
    tp2.mem_num = 4*16*8*16/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;

    req.task_submit = tp2;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 2);
        return false;
    }
    sleep(1);

    //task3 - tcp connections
    struct subtask_params tp3;
    strcpy(tp3.name, "normal_TEST1_tcp_cons");
    tp3.obj_num = 1;
    tp3.objs[0] = *obj0;
    strcpy(tp3.filename, "tcp_cons.so");
    // tp3.filename = "tcp_cons.so";
    tp3.pi_num = 1;
    struct plugin_params p_params31 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "tcp_cons.so",
        .funcname = "plugin_0"
    };
    tp3.pis[0] = p_params31;
    tp3.time = 1800;
    tp3.epoch = 60;
    tp3.cpu_num = 1;
    tp3.mem_num = 4*16*8*16/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;

    req.task_submit = tp3;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 3);
        return false;
    }
    sleep(1);

    //task4 - dump
    struct subtask_params tp4;
    strcpy(tp4.name, "normal_TEST1_dump");
    tp4.obj_num = 1;
    tp4.objs[0] = *obj0;
    strcpy(tp4.filename, "dump.so");
    tp4.pi_num = 1;
    struct plugin_params p_params41 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "dump.so",
        .funcname = "plugin_0"
    };
    tp4.pis[0] = p_params41;
    tp4.time = 1800;
    tp4.epoch = 60;
    tp4.cpu_num = 1;
    tp4.mem_num = 4*16*8*16/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;
    req.task_submit = tp4;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 4);
        return false;
    }
    
    #endif


    #ifdef TEST2
    //task0 - baseline
    struct subtask_params tp0;
    strcpy(tp0.name, "normal_TEST2_baseline");
    tp0.obj_num = 1;
    tp0.objs[0] = *obj0;
    tp0.obj_split = true;
    tp0.can_migrate = true;
    strcpy(tp0.filename, "baseline.so");
    tp0.pi_num = 1;
    struct plugin_params p_params01 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "baseline.so",
        .funcname = "plugin_0"
    };
    tp0.pis[0] = p_params01;
    tp0.pi_relations[0][0] = -1;
    tp0.time = 600;
    tp0.epoch = 60;
    tp0.cpu_num = 1;
    tp0.mem_num = 1*1*2*4/1024;
    tp0.low = 0;
    tp0.high = 0;
    tp0.range = 1;
    //task1 - print packet info
    struct subtask_params tp1;
    strcpy(tp1.name, "normal_TEST2_task1");
    tp1.obj_num = 2;
    tp1.objs[0] = *obj110;
    tp1.objs[1] = *obj111;
    tp1.obj_split = true;
    tp1.can_migrate = true;
    strcpy(tp1.filename, "task1.so");
    tp1.pi_num = 1;
    struct plugin_params p_params11 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "task1.so",
        .funcname = "plugin_0"
    };
    tp1.pis[0] = p_params11;
    tp1.pi_relations[0][0] = -1;
    tp1.time = 600;
    tp1.epoch = 60;
    tp1.cpu_num = 1;
    tp1.mem_num = 1*1*2*4/1024;
    tp1.low = 0;
    tp1.high = 0;
    tp1.range = 1;
                                                            
    //task2 - heavy hitter detection
    struct subtask_params tp2;
    strcpy(tp2.name, "normal_TEST2_task2");
    tp2.obj_num = 2;
    tp2.objs[0] = *obj120;
    tp2.objs[1] = *obj121;
    tp2.obj_split = true;
    tp2.can_migrate = true;
    strcpy(tp2.filename, "task2.so");
    tp2.pi_num = 2;
    struct plugin_params p_params21 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_0"
    };
    tp2.pis[0] = p_params21;
    struct plugin_params p_params22 = {
        .plugin_index = 1,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 128,
            .bucketsize = 5,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_1"
    };
    tp2.pis[1] = p_params22;
    tp2.pi_relations[0][0] = -1;
    tp2.time = 600;
    tp2.epoch = 60;
    tp2.cpu_num = 1;
    tp2.mem_num = 4*16*8*16/1024;
    tp2.low = 0;
    tp2.high = 0;
    tp2.range = 1;

    //task3 - tcp connections
    struct subtask_params tp3;
    strcpy(tp3.name, "normal_TEST2_tcp_cons");
    tp3.obj_num = 2;
    tp3.objs[0] = *obj130;
    tp3.objs[1] = *obj131;
    tp3.obj_split = true;
    tp3.can_migrate = true;
    strcpy(tp3.filename, "tcp_cons.so");
    // tp3.filename = "tcp_cons.so";
    tp3.pi_num = 2;
    struct plugin_params p_params31 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "tcp_cons.so",
        .funcname = "plugin_0"
    };
    tp3.pis[0] = p_params31;
    
    tp3.pi_relations[0][0] = -1;
    tp3.time = 600;
    tp3.epoch = 60;
    tp3.cpu_num = 1;
    tp3.mem_num = 4*16*8*16/1024;
    tp3.low = 0;
    tp3.high = 0;
    tp3.range = 1;

    //task4 - dump
    struct subtask_params tp4;
    strcpy(tp4.name, "normal_TEST2_dump");
    tp4.obj_num = 2;
    tp4.objs[0] = *obj140;
    tp4.objs[1] = *obj141;
    tp4.obj_split = true;
    tp4.can_migrate = true;
    strcpy(tp4.filename, "dump.so");
    tp4.pi_num = 2;
    struct plugin_params p_params41 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "dump.so",
        .funcname = "plugin_0"
    };
    tp4.pis[0] = p_params41;
    
    tp4.pi_relations[0][0] = -1;
    tp4.time = 600;
    tp4.epoch = 60;
    tp4.cpu_num = 1;
    tp4.mem_num = 4*16*8*16/1024;
    tp4.low = 0;
    tp4.high = 0;
    tp4.range = 1;


    process_task(tp0);
    
    process_task(tp1);
    
    process_task(tp2);
    
    process_task(tp3);
  
    process_task(tp4);
    #endif


    #ifdef TEST3
    //task0 - baseline
    struct subtask_params tp0;
    strcpy(tp0.name, "normal_TEST1_baseline");
    tp0.obj_num = 1;
    tp0.objs[0] = *obj0;
    strcpy(tp0.filename, "baseline.so");
    tp0.pi_num = 1;
    struct plugin_params p_params01 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "baseline.so",
        .funcname = "plugin_0"
    };
    tp0.pis[0] = p_params01;
    tp0.time = 1800;
    tp0.epoch = 60;
    tp0.cpu_num = 2;
    tp0.mem_num = 1*1*2*4/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;
    struct command_req req;
    struct command_rsp rsp;
    req.type = TASK_SUBMIT;
    req.task_submit = tp0;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 0);
        return false;
    }

    //task1 - print packet info
    struct subtask_params tp1;
    strcpy(tp1.name, "normal_TEST1_task1");
    tp1.obj_num = 1;
    tp1.objs[0] = *obj0;

    strcpy(tp1.filename, "task1.so");
    tp1.pi_num = 1;
    struct plugin_params p_params11 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "task1.so",
        .funcname = "plugin_0"
    };
    tp1.pis[0] = p_params11;
    tp1.time = 1800;
    tp1.epoch = 60;
    tp1.cpu_num = 1;
    tp1.mem_num = 1*1*2*4/1024;
    tp1.run_actor_times_per_manage = 128;
    tp1.retrycount = 0;
    tp1.retry_time = 0;

    req.task_submit = tp1;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 1);
        return false;
    }
    //task2 - heavy hitter detection
    struct subtask_params tp2;
    strcpy(tp2.name, "normal_TEST1_task2");
    tp2.obj_num = 1;
    tp2.objs[0] = *obj22;
    strcpy(tp2.filename, "task2.so");
    tp2.pi_num = 2;
    struct plugin_params p_params21 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_0"
    };
    tp2.pis[0] = p_params21;
    struct plugin_params p_params22 = {
        .plugin_index = 1,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 128,
            .bucketsize = 5,
            .countersize = 4
        },
        .filename = "task2.so",
        .funcname = "plugin_1"
    };
    tp2.pis[1] = p_params22;
    tp2.time = 1800;
    tp2.epoch = 60;
    tp2.cpu_num = 1;
    tp2.mem_num = 4*16*8*16/1024;
    tp2.run_actor_times_per_manage = 128;
    tp2.retrycount = 0;
    tp2.retry_time = 0;

    req.task_submit = tp2;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 2);
        return false;
    }

    //task3 - tcp connections
    struct subtask_params tp3;
    strcpy(tp3.name, "normal_TEST1_tcp_cons");
    tp3.obj_num = 1;
    tp3.objs[0] = *obj23;
    strcpy(tp3.filename, "tcp_cons.so");
    // tp3.filename = "tcp_cons.so";
    tp3.pi_num = 1;
    struct plugin_params p_params31 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "tcp_cons.so",
        .funcname = "plugin_0"
    };
    tp3.pis[0] = p_params31;
    tp3.time = 1800;
    tp3.epoch = 60;
    tp3.cpu_num = 4;
    tp3.mem_num = 4*16*8*16/1024;
    tp3.run_actor_times_per_manage = 128;
    tp3.retrycount = 0;
    tp3.retry_time = 0;

    req.task_submit = tp3;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 3);
        return false;
    }

    //task4 - dump
    struct subtask_params tp4;
    strcpy(tp4.name, "normal_TEST1_dump");
    tp4.obj_num = 1;
    tp4.objs[0] = *obj24;
    strcpy(tp4.filename, "dump.so");
    tp4.pi_num = 1;
    struct plugin_params p_params41 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "dump.so",
        .funcname = "plugin_0"
    };
    tp4.pis[0] = p_params41;
    tp4.time = 100;
    tp4.epoch = 60;
    tp4.cpu_num = 2;
    tp4.mem_num = 4*16*8*16/1024;
    tp4.run_actor_times_per_manage = 128;
    tp4.retrycount = 0;
    tp4.retry_time = 0;
    req.task_submit = tp4;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 4);
        return false;
    }
    
    #endif

    #ifdef TEST4
    struct subtask_params tp0;
    strcpy(tp0.name, "normal_TEST1_baseline");
    tp0.obj_num = 1;
    tp0.objs[0] = *obj23;
    strcpy(tp0.filename, "baseline.so");
    tp0.pi_num = 1;
    struct plugin_params p_params01 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 1,
            .bucketnum = 1,
            .bucketsize = 2,
            .countersize = 4
        },
        .filename = "baseline.so",
        .funcname = "plugin_0"
    };
    tp0.pis[0] = p_params01;
    tp0.time = 1800;
    tp0.epoch = 60;
    tp0.cpu_num = 2;
    tp0.mem_num = 1*1*2*4/1024;
    tp0.run_actor_times_per_manage = 128;
    tp0.retrycount = 0;
    tp0.retry_time = 0;
    struct command_req req;
    struct command_rsp rsp;
    req.type = TASK_SUBMIT;
    req.task_submit = tp0;
    if(!send_command(NULL,-1,&req,&rsp)){
        printf("send command error in tp %d\n", 0);
        return false;
    }
    #endif
    #ifdef TEST5
    init_objects();
    int num;
    InterObject **objs = read_interobject(&num);
    // for(int i = 0; i < num; i++){
    //     print_intersection_object(objs[i]);
    // }
    // for(int i=0; i<num; i++){
    //     printf("task %d\n",i);
    //     update_CS_per_intersection_obj(objs[i],i);
    //     print_s();
    // }
    for(int i=0;i<10;i++){
        struct subtask_params tp2;
        strcpy(tp2.name, "custom_task");
        tp2.obj_num = 1;
        tp2.objs[0] = *objs[i];
        strcpy(tp2.filename, "custom_task.so");
        tp2.pi_num = 1;
        struct plugin_params p_params21 = {
            .plugin_index = 0,
            .cnt_info = {
                .rownum = 8,
                .bucketnum = 32,
                .bucketsize = 1,
                .countersize = 4
            },
            .filename = "custom_task.so",
            .funcname = "plugin_0"
        };
        tp2.pis[0] = p_params21;
        tp2.time = 1800;
        tp2.epoch = 60;
        tp2.cpu_num = 1;
        tp2.mem_num = 8*32*1*4/1024;
        tp2.run_actor_times_per_manage = 128;
        tp2.retrycount = 0;
        tp2.retry_time = 0;
        struct command_req req;
        struct command_rsp rsp;
        req.type = TASK_SUBMIT;
        req.task_submit = tp2;
        if(!send_command(NULL,-1,&req,&rsp)){
            printf("send command error in tp %d\n", 1);
            return false;
        }
    }
    #endif
    #ifdef TEST6
    init_objects();
    int num;
    InterObject **objs = read_interobject(&num);
    for(int i=0;i<10;i++){
        struct subtask_params tp2;
        sprintf(tp2.name, "custom_task_1%d", i); // Add i to the task name
        tp2.obj_num = 1;     
        tp2.objs[0] = *objs[i];
        // sprintf(tp2.filename, "custom_task_no_prefilter1%d.so",i);
        sprintf(tp2.filename, "custom_task_1%d.so",i);
        tp2.pi_num = 1;
        struct plugin_params p_params21 = {
            .plugin_index = 0,
            .cnt_info = {
                .rownum = 8,
                .bucketnum = 32,
                .bucketsize = 1,
                .countersize = 4
            },
            .funcname = "plugin_0"
        };
        strcpy(p_params21.filename, tp2.filename);
        tp2.pis[0] = p_params21;
        tp2.time = 1800;
        tp2.epoch = 60;
        tp2.cpu_num = 1;
        tp2.mem_num = 8*32*1*4/1024;
        tp2.run_actor_times_per_manage = 128;
        tp2.retrycount = 0;
        tp2.retry_time = 0;
        struct command_req req;
        struct command_rsp rsp;
        req.type = TASK_SUBMIT;
        req.task_submit = tp2;
        if(!send_command(NULL,-1,&req,&rsp)){
            printf("send command error in tp %d\n", 1);
            return false;
        }
        sleep(1);
    }
    
    #endif
}