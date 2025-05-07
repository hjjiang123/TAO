#include <stdio.h>
#include "task.h"
#include "subtask.h"
#include <rte_lcore.h>

typedef struct result_back {
    int epoch;
    void *handle;
    int run_actor_times_per_manage;
    uint64_t arrive_time;
    uint64_t retired_time;
    bool is_retired;
    float last_load;
    int last_load_type;
} ResultBack;


void get_task_result();