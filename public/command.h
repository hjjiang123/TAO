#ifndef COMMAND_H
#define COMMAND_H
#include "param.h"
#include "object.h"


enum command_req_type {
    TASK_SUBMIT   =0,
};

struct command_req {
    enum command_req_type type;
    union {
        struct subtask_params task_submit; //for TASK_SUBMIT
    };
};

struct command_rsp {
    int status;
    union {
        struct {
            unsigned long long c_percore;
        } monitor_rsp;
    };
    
};
bool send_command(char *host_ip, int port, struct command_req* req, struct command_rsp* rsp);
#endif