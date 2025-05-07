#ifndef TASK_H
#define TASK_H
#include "object.h"
#include "param.h"


//任务
typedef struct task {
    char name[NAME_MAX]; //任务名
    int task_id; //任务号，由任务注册分配
    int cpu_num; //用户预计cpu数量
    int mem_num; //用户预计内存数量
    int subtask_num; //子任务数量,生成子任务时确定
    int obj_num; //对象数量
    InterObject **objs; //对象数组
    int d_num; //对象差集数量
    DifferObject **d; //对象差集数组

    int obj_split; //目标可分性
    bool can_migrate; //是否可迁移
    char filename[100];  /** task Plugin filename */
    int pi_num; //插件数量
    struct plugin_params pis[MAX_PLUGIN_NUM_ONETASK]; //插件数组
    int pi_relations[MAX_PLUGIN_NUM_ONETASK][MAX_PLUGIN_NUM_ONETASK]; //插件DAG
    long long time; //任务持续秒数
    int epoch; //测量周期，秒数

    //for input_thread pre-dropping
    int low;
    int high;
    int range;

    void *handle;
    
} task;
#endif