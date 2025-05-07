#ifndef SUBTASK_H
#define SUBTASK_H
#include "task.h"
#include "object.h"
#include "param.h"
#include "statistics.h"
#include "publish_middlebox.h"
#include "kafka_p_c.h"
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <stddef.h>
#include <rte_ring.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_hash_crc.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <pthread.h>
#include <math.h>


#define MAX_ID 128
#define MAX_PLUGIN_ARGS_NUM 8
#ifndef ACTOR_MSGQ_SIZE
#define ACTOR_MSGQ_SIZE        64
#endif
#define MAX_DISPATCH_BUFFER_SIZE 16
// #ifndef STATISTICS1
// #define STATISTICS1
// #endif
#ifndef STATISTICS1
#define STATISTICS1
#endif
#define MAX_STATISTICS1_NUM 102400

extern pthread_mutex_t subtask_lock; // 定义互斥锁

int get_count_transfer_between_tasks(int k, int i,int j);
typedef char Byte;
typedef int (*PF)(struct rte_mbuf *, Byte ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[MAX_PLUGIN_ARGS_NUM],kafka_params *k_params); //packet processing function
typedef int (*PF1)(ResultData *, Byte ****res, int rownum, int bucketnum,int bucketsize,int countersize, void *to_next[MAX_PLUGIN_ARGS_NUM],ResultDataRing *res_ring,rd_kafka_t* rk,char topic[NAME_MAX],int *loss); //ResultData processing function

struct plugin {
    struct CounterInfo cnt_info;
    int plugin_index;
    Byte ****res;
    PF func;
    void *handle;
};

typedef void (*DISPATCH)(int thread, int task_id,MARKID *markid,struct rte_mbuf *pkt);

struct pipeline
{
    void *to_next[MAX_PLUGIN_ARGS_NUM];
    int plugin_num;
    struct plugin *plugins[MAX_PLUGIN_NUM_ONEPIPELINE];
    // uint32_t timer_period_ms;
    // bool flipable;
    // bool is_input;
    
    kafka_params *k_params;
    // ResultDataRing *res_ring;
    // rd_kafka_t* rk;
    // char topic[NAME_MAX];

    struct pipeline_statistics stat;
};


struct actor {
    int global_actor_id;
	int actor_id;
    int task_id;
    char task_name[NAME_MAX];
	struct rte_ring *msgq_req;
	struct rte_ring *msgq_rsp;
    
	struct rte_ring * ring_in;
	struct pipeline *p;

	DISPATCH dispatch;
    int thread_id; // decided by scheduler

	// for manage
	int run_actor_times_per_manage; // 表示每执行x次actor pipeline就读取一次管理报文
	int run_actor_times; // 表示当前执行了多少次actor pipeline,初始化为0,每执行一次加1

    pthread_mutex_t lock; // 定义互斥锁,用于管理actor修改操作
    bool is_exclusive; // true: exclusive, false: shared
    // ONLY READ/WRITE BY THREAD
    uint64_t retired_time; //任务过期CPU时间戳
    bool is_retired;

    //for schedule
    int cooldown; //迁移冷却时间（调度轮数）
    float load;
    // statistics
    #ifdef STATISTICS1
    int sta_index;
    double *time_shot;
    #endif
};

struct subtask {
    TAILQ_ENTRY(subtask) node;
    char name[NAME_MAX];
    int task_id;
    int res_ring_size; //结果环大小
    int actor_num;
    /**** for running ****/
    struct actor *actors[MAX_SUBTASK_CORE_NUM];
    pthread_mutex_t lock; // 定义互斥锁,用于管理actors修改操作

    /**** attributes****/ 
    int cpu; 
    int mem;
    
    // struct actor_params *at_params;

    int obj_num; //对象数量
    InterObject **objs; //对象数组
    int d_num; //对象差集数量
    DifferObject **d; //对象差集数组
    
    char filename[NAME_SIZE];  /** task Plugin filename */
    int pi_num; //插件数量
    struct plugin_params pis[MAX_PLUGIN_NUM_ONETASK]; //插件数组

    uint64_t time; //任务持续秒数
     
    int epoch; //测量周期，秒数
    void *handle;
    
    /**** for manage****/ 
	int run_actor_times_per_manage; // 表示每执行x次actor pipeline就读取一次管理报文

    /**** for scheduling****/ 
    uint64_t arrive_time;
    uint64_t retired_time; //任务过期CPU时间戳
    bool is_retired;
    // for increase or decrease actor
    float last_load; //上一调度周期的最大负载
    int last_load_type; //used for rescheduling2
};

void init_task_id_system();
int get_available_task_id();
void init_actor_id_system();

TAILQ_HEAD(subtask_list, subtask);

unsigned int calculate_core_num(float cpu); 

int subtask_init(void);

struct subtask *
subtask_find(int task_id);
struct subtask *get_next_subtask(struct subtask *st);
struct subtask *get_earliest_subtask_after_time(uint64_t time);

struct subtask *
subtask_create(struct subtask_params *params);

int subtask_free(struct subtask *subtask);

struct actor *
actor_find(int task_id, int actor_id);

struct actor *
actor_create(struct actor_params *params);

int actor_enable(struct subtask *subtask, 
                    struct actor *p);
 
int actor_disable(struct actor *p);

int actor_free(struct actor *p);

struct plugin *
plugin_create(struct pipeline *p, 
                            struct plugin_params *params);

int plugin_free(struct pipeline *p, struct plugin *plugin);

int run_pipeline(struct pipeline *p, struct rte_mbuf *pkt);

void inter_dispatch(int thread_id,int task_id,MARKID *markid,struct rte_mbuf *pkt);

// void intra_dispatch(int cpu_id,int task_id,MARKID *markid,struct rte_mbuf *pkt);
void print_AB(int thread_id);
void update_A(int cpu_id, int **newA);
void updateA_add(int cpu_id, int object_id,int task_id);
void updateA_del(int cpu_id, int object_id,int task_id);
void updateA_edit(int cpu_id, int object_id,int task_id,int next_task_id);
void prune_A(int **newA);
int readA(int cpu_id, int object_id,int task_id);
// void updateB_add_bulk(int cpu_id, int task_id,int multi_pipeline_num,struct rte_ring **rings);
int find_B_loss(int cpu_id,int task_id,int actor_id);
void copy_B_loss(int thread_id, int **loss);
int find_thread_loss(int cpu_id);
void updateB_del(int cpu_id, int task_id,struct rte_ring *ring);
void updateB_del_task(int cpu_id, int task_id);
void updateB_add(int cpu_id,int task_id,struct rte_ring *ring);
void get_packet_addr(struct rte_mbuf* mbuf, uint32_t* src_addr, uint32_t* dst_addr);
uint32_t calculate_hash(const uint32_t* src_addr, const uint32_t* dst_addr);
#endif