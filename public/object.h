#ifndef OBJECT_H
#define OBJECT_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define MAX_HOST_NUM 3
#define MAX_INTERSECTION_NUM_PER_DIFFERENCE 32
#define MAX_INTERSECTION_NUM 256
#define MAX_OBJECTS_NUM_PER_TASK 32
#define MAX_DIFFERENCE_NUM_PER_TASK 16
#define MAX_PRIORITY 16
#define MAX_TASK_NUM 64

//对象交集
typedef struct IntersectionObject {
    int direction;           // 0->in, 1->bi
    unsigned int src_ip;      //源地址
    unsigned int src_mask;    //源地址掩码
    unsigned int dst_ip;     //宿地址
    unsigned int dst_mask;   //宿地址掩码
    unsigned short src_port;  //源地址端口
    unsigned short dst_port; //宿地址端口
    unsigned char protocol;  //协议

    unsigned int priority;   //优先级
    unsigned int parrent_obj_id; //父对象id,用于维持对象交集添加时流量分发的一致性，一次性属性，对象删除时忽略它
    bool is_new; //是否是新产生的对象交集
    int task_ids[MAX_PRIORITY]; //任务id
    int hosts[MAX_HOST_NUM]; //主机id

    unsigned long long packets; //包数
    unsigned long long mtime;   //测量时间
    
} InterObject;

int mask_to_prefix_length(unsigned int mask);

/*对象交集操作*/
//判断两个对象交集是否相等
bool equals_intersection_object(InterObject *obj1, InterObject *obj2);

//获取对象交集的交集
bool get_intersection_object(InterObject *obj1, InterObject *obj2,InterObject *obj3);

//obj1是否包含obj2
bool contain_intersection_object_or_not(InterObject *obj1, InterObject *obj2);

//交集对象obj的优先级增加1
void increase_intersection_object_priority(InterObject *obj);

//打印对象交集
void print_intersection_object(InterObject *obj);

/*完全对象交集集合操作*/

//获取对象交集集合中的对象的编号
int find_intersection_object(InterObject *obj);

//获得所有对象交集的序号
int get_all_intersection_object_index(int index_sequence[MAX_INTERSECTION_NUM]);

//获取对象交集集合中的对象
InterObject *intersection_object(int index);
//添加对象交集
int add_intersection_object(InterObject *obj);
//删除对象交集
int remove_intersection_object(InterObject *obj);

int get_least_priority_intersection_object();
int get_next_least_priority_intersection_object(int cur);
int sort_interset_objects(int index_sequence[], InterObject** obj_set, int num);
void end_add_s();
int **produce_global_A();

/*对象差集*/
typedef struct DifferenceObject {
    InterObject *base_interset;
    int sub_interset_num;
    InterObject *sub_intersets[MAX_INTERSECTION_NUM_PER_DIFFERENCE];
} DifferObject;

/*对象差集操作*/

/*对象差集集合操作*/
DifferObject *find_difference_object(int index);

void init_objects();
void print_intersection_object(InterObject *obj);
void print_difference_object(DifferObject *obj);
void print_s();
void print_d();

// update algorithm
void update_CS_per_intersection_obj(InterObject *origin_obj, int task_id);
void remove_CS_per_intersection_obj(InterObject *ojk, int task_id);
int add_task_all_CS(int task_id, InterObject **origin_obj, int num,DifferObject **do_jk);

int remove_task_all_CS(int task_id, InterObject **Oj,int num);

#endif