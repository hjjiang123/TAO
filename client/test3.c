#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "object.h"
#include "param.h"
#include "command.h"

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

    #ifdef TEST1
    struct subtask_params tp2;
    strcpy(tp2.name, "task3");// Add i to the task name
    tp2.obj_num = 1;     
    tp2.objs[0] = *obj0;
    // sprintf(tp2.filename, "custom_task_no_prefilter1%d.so",i);
    strcpy(tp2.filename, "task3.so");
    tp2.res_ring_size = 128;
    tp2.pi_num = 1;
    struct plugin_params p_params21 = {
        .plugin_index = 0,
        .cnt_info = {
            .rownum = 8,
            .bucketnum = 32,
            .bucketsize = 1,
            .countersize = 4
        },
        .filename = "task3.so",
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
    
    #endif

}