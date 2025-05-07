#ifndef PARAM_H
#define PARAM_H
#include <stdint.h>
#include "object.h"

#define DPDK_PORT                       1
#define MAX_PLUGIN_NUM                  8
#define MAX_SUBTASK_CORE_NUM            32
#define MAX_CORE_NUM                    32
#define MAX_FLOWENTRY_NUM_ONETASK       8
#define MAX_PRI_FLOW_NUM_PER_FLOWENTRY  4
#define MAX_FLOW_NUM_PER_OBJECT         4
#define NAME_SIZE                       64
#define BURST_SIZE                      256
#define PIPELINE_MSGQ_SIZE              64
#define RING_SIZE                       16384
#define MAX_PLUGIN_NUM_ONEPIPELINE      8
#define MAX_PLUGIN_NUM_ONETASK          16
#ifndef NAME_MAX
#define NAME_MAX                        255
#endif
#define MAX_MULTI_PIPELINE_NUM 16

#define MAX_FLOW_DIRECTOR_NUM 64
#define NODE_PORT 62345

// ryu restful api
#define POSTURL "http://211.65.193.243:18080/"
#define POST_COMMAND_GROUPENTRY_ADD "stats/groupentry/add"
#define POST_COMMAND_GROUPENTRY_DELETE "stats/groupentry/delete"
#define POST_COMMAND_GROUPENTRY_MODIFY "stats/groupentry/modify"
#define POST_COMMAND_FLOWENTRY_ADD "stats/flowentry/add"
#define POST_COMMAND_FLOWENTRY_DELETE "stats/flowentry/delete"
#define POST_COMMAND_FLOWENTRY_MODIFY "stats/flowentry/modify"
#define POST_COMMAND_FLOWENTRY_QUERY "stats/flow/119839591101537"
#define NUM_MBUFS_DEFAULT 1024*8

#define BROKER "211.65.193.243:30092"
#define MASTER_PLUGIN_DIR "/home/hjjiang/hjjiang_capture_v3/master/task_examples"
#define NODE_PLUGIN_DIR "/home/hjjiang/hjjiang_capture_v3/node/task_examples"

#define THREAD_INPUT_THREAD_NUM 2 //输入线程数量
#define SCHEDULE_MAX_K 3; // Number of retries before removing a task
#define SCHEDULE_WAIT_TIME 3; // Number of seconds to wait before retrying a task
#define SCHEDULE_SLEEP_TIME 5 // rescheduling duration
#define SCHEDULE_THETA_1 0.2 //重压力阈值
// #define SCHEDULE_THETA_2 0.05 //轻压力阈值
#define SCHEDULE_THETA_3 0.7 //重负载阈值
#define SCHEDULE_THETA_4 0.35 //轻负载阈值
#define SCHEDULE_THETA_5 0.35 //当多actor在共享同一线程时，每actor重负载阈值
#define SCHEDULE_UP_TOLERANCE 0.1 //允许负载短暂地超出重负载阈值
#define SCHEDULE_DOWN_TOLERANCE 0.05 //允许负载短暂地低于轻负载阈值
#define SCHEDULE_LOSS_THRESHOLD 0.05 //丢包率阈值
#define SCHEDULE_MIN_LOAD_IN_NORMAL_TASK 0.1 //正常负载任务的最小负载阈值
#define SCHEDULE_COOLDOWN 3
#define SCHEDULE_MIN_STABLE_ACTOR_NUM 3
#define SCHEDULE_MAX_STABLE_ACTOR_NUM 5
#define SCHEDULER_MAX_ACTOR_NUM 64


#define SCHEDULE2_WAIT_TIME 3; // Number of seconds to wait before retrying a task
#define SCHEDULE2_SLEEP_TIME 3 // rescheduling duration

#define SCHEDULE2_P_THETA_1 0.2 //重压力阈值

#define SCHEDULE2_L_THETA_2 0.7 //重/超重负载阈值
#define SCHEDULE2_L_THETA_3 0.6 //正常/重负载阈值
#define SCHEDULE2_L_THETA_4 0.35 //轻/正常负载阈值
#define SCHEDULE2_L_THETA_5 0.25 //超轻/轻负载阈值

#define SCHEDULE2_L_THETA_SCALE 1 //当多actor在共享同一线程时，各负载阈值的缩小比例


#define SCHEDULE2_UP_TOLERANCE 0.1 //允许负载短暂地超出重负载阈值
#define SCHEDULE2_DOWN_TOLERANCE 0.1 //允许负载短暂地低于轻负载阈值
#define SCHEDULE2_LOSS_THRESHOLD 0.05 //丢包率阈值
#define SCHEDULE2_MIN_LOAD_IN_NORMAL_TASK 0.1 //正常负载任务的最小负载阈值
#define SCHEDULE2_COOLDOWN 3
#define SCHEDULE2_MIN_STABLE_ACTOR_NUM 3
#define SCHEDULE2_MAX_STABLE_ACTOR_NUM 5
#define SCHEDULER2_MAX_ACTOR_NUM 64



#define SUBTASK_DISPATCH_FAILURE_RETRY_TIMES 3

typedef union
{
    struct
    {
        uint16_t hash;
        uint16_t object_id;
    } id1;
    unsigned int id2;
} MARKID;

typedef struct B_Array
{
    int cur;
    struct rte_ring *rings[MAX_MULTI_PIPELINE_NUM];
    int loss[MAX_MULTI_PIPELINE_NUM];
} B_Array;

struct CounterInfo
{
    int rownum;
    int bucketnum;
    int bucketsize;
    int countersize;
};

struct plugin_params {
    int plugin_index;
    struct CounterInfo cnt_info;
    char filename[NAME_SIZE];
    char funcname[NAME_SIZE];
};

struct actor_params
{
    int task_id;
    // bool flipable;
    // bool is_input;
    
    // for manage
    int run_actor_times_per_manage; // 表示每执行x次actor pipeline就读取一次管理报文
    int ring_size;
    int res_ring_size; //结果环大小
    int plugin_num;
    struct plugin_params plugin_params[MAX_PLUGIN_NUM];
};
struct subtask_params {
    char name[NAME_MAX];
    
    int obj_num; //流规则数量
    InterObject objs[MAX_OBJECTS_NUM_PER_TASK]; //流规则数组
    char filename[NAME_SIZE];  /** task Plugin filename */
    int res_ring_size; //结果环大小
    int pi_num; //插件数量
    struct plugin_params pis[MAX_PLUGIN_NUM_ONETASK]; //插件数组
    uint64_t time; //任务持续秒数
    int epoch; //测量周期，秒数
    int cpu_num; //用户预计cpu数量
    int mem_num; //用户预计内存数量
    
    // for manage
    int run_actor_times_per_manage; // 表示每执行x次actor pipeline就读取一次管理报文
    
    // for schedule
    int retrycount; // 尝试调度失败次数，init to 0
    int retry_time; // 下次尝试调度的时间，init to 0
};

struct task_param{
    char name[NAME_MAX]; 
    int obj_num; //流规则数量
    InterObject objs[MAX_OBJECTS_NUM_PER_TASK]; //流规则数组
    bool obj_split; //目标可分性
    bool can_migrate; //是否可迁移
    char filename[NAME_SIZE];  /** task Plugin filename */
    
    int pi_num; //插件数量
    struct plugin_params pis[MAX_PLUGIN_NUM_ONETASK]; //插件数组
    int pi_relations[MAX_PLUGIN_NUM_ONETASK][MAX_PLUGIN_NUM_ONETASK]; //插件DAG
    long long time; //任务持续秒数
    int epoch; //测量周期，秒数
    int cpu_num; //用户预计cpu数量
    int mem_num; //用户预计内存数量


    //extral param
    float c;
};
#endif