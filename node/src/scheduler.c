#include "scheduler.h"
#include "thread.h"
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <pthread.h>

// Define the structure for the task queue node
struct task_queue_node {
    struct subtask_params param;
    struct task_queue_node* next;
};

// Define the structure for the task queue
struct task_queue {
    struct task_queue_node* head;
    struct task_queue_node* tail;
    pthread_mutex_t lock; // Mutex lock for synchronization
};

struct actor_statistic{
    int task_id;
    int actor_id;
    int global_actor_id;
    int thread_id;
    uint64_t cycles;
    uint64_t cycles_dispatch;
    uint32_t read_packets;
    int num_0; //取得0个报文的次数
    int num_1; //取得1个报文的次数
    int num_2; //取得2个及以上报文的次数
    int loss;
    int recv_loss;
    int max_ring_len;
    bool is_retired;
    bool is_exclusive;
    float load;
    float pressure;
};

// Function to initialize the task queue

struct task_queue * TaskList;
struct task_queue * wait_queue;

void init_task_queue(struct task_queue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->lock, NULL); // Initialize the mutex lock
}

// Function to check if the task queue is empty
bool is_task_queue_empty(struct task_queue* queue) {
    return queue->head == NULL;
}
int len_task_queue(struct task_queue* queue) {
    int len = 0;
    struct task_queue_node* current = queue->head;
    while (current != NULL) {
        len++;
        current = current->next;
    }
    return len;
}

// Function to enqueue a task parameter
void enqueue_subtask_params(struct task_queue* queue, struct subtask_params param) {
    // Create a new node for the task parameter
    struct task_queue_node* new_node = (struct task_queue_node*)malloc(sizeof(struct task_queue_node));
    new_node->param = param;
    new_node->next = NULL;
    
    pthread_mutex_lock(&queue->lock); // Acquire the lock

    // If the queue is empty, set the new node as both head and tail
    if (is_task_queue_empty(queue)) {
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        // Otherwise, add the new node to the tail of the queue
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    #ifdef DEBUG
    printf("schedule - Enqueued task: %s\n", param.name);
    printf("schedule - len_task_queue(queue): %d\n", len_task_queue(queue));
    #endif
    pthread_mutex_unlock(&queue->lock); // Release the lock
}

void sche_enqueue_subtask_params(struct subtask_params param) {
    enqueue_subtask_params(TaskList, param);
}

// Function to dequeue a task parameter
struct subtask_params dequeue_subtask_params(struct task_queue* queue) {
    pthread_mutex_lock(&queue->lock); // Acquire the lock
    // Check if the queue is empty
    
    if (is_task_queue_empty(queue)) {
        // Return a default task parameter or handle the error as needed
        struct subtask_params default_param;
        memcpy(default_param.name, "", sizeof(""));
        pthread_mutex_unlock(&queue->lock); // Release the lock
        return default_param;
    }
    // Get the task parameter from the head of the queue
    struct subtask_params param = queue->head->param;

    // Remove the head node from the queue
    struct task_queue_node* temp = queue->head;
    queue->head = queue->head->next;
    free(temp);

    // If the queue becomes empty, update the tail pointer
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    #ifdef DEBUG
    printf("schedule - Dequeued task: %s\n", param.name);
    printf("schedule - len_task_queue(queue): %d\n", len_task_queue(queue));
    #endif
    pthread_mutex_unlock(&queue->lock); // Release the lock

    return param;
}

struct subtask_params dequeue_subtask_params_with_condition(struct task_queue* queue, bool (*condition)(struct subtask_params*)) {
    
    pthread_mutex_lock(&queue->lock); // Acquire the lock
    // Check if the queue is empty
    if (is_task_queue_empty(queue)) {
        // Return a default task parameter or handle the error as needed
        struct subtask_params default_param;
        memcpy(default_param.name, "", sizeof(""));
        pthread_mutex_unlock(&queue->lock); // Release the lock
        return default_param;
    }
    
    // Traverse the queue to find a subtask_params that satisfies the condition
    struct task_queue_node* current = queue->head;
    struct task_queue_node* previous = NULL;
    while (current != NULL) {
        if (condition(&(current->param))) {
            // Found a subtask_params that satisfies the condition
            struct subtask_params param = current->param;

            // Remove the node from the queue
            if (previous == NULL) {
                // The node is the head of the queue
                queue->head = current->next;
            } else {
                // The node is not the head of the queue
                previous->next = current->next;
            }

            // Update the tail pointer if necessary
            if (current == queue->tail) {
                queue->tail = previous;
            }

            // Free the memory of the node
            free(current);

            pthread_mutex_unlock(&queue->lock); // Release the lock

            return param;
        }

        // Move to the next node
        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&queue->lock); // Release the lock

    // No subtask_params satisfies the condition, return a default task parameter or handle the error as needed
    struct subtask_params default_param;
    memcpy(default_param.name, "", sizeof(""));
    
    return default_param;
}

void task_register(int **A) {
    int i;
	RTE_LCORE_FOREACH_WORKER(i) {
        if(i==0 || i==1 || i>=MAX_CORE_NUM){
            continue;
        }
        struct thread_msg_req q;
        q.type = THREAD_REQ_UPDATE_A;
        q.A = A;
        struct thread_msg_rsp *rsp1 = thread_msg_send_recv(i, &q);
        thread_msg_free(rsp1);
	}
}

void delete_B(int task_id) {
    int i;
    RTE_LCORE_FOREACH_WORKER(i) {
        if(i==0 || i==1 || i>=MAX_CORE_NUM){
            continue;
        }
        struct thread_msg_req q;
        q.type = THREAD_REQ_B_UPDATE_DEL_TASK;
        q.task_id = task_id;
        struct thread_msg_rsp *rsp1 = thread_msg_send_recv(i, &q);
        thread_msg_free(rsp1);
    }
}
int times; // for experiment
bool flag; // for experiment
int last_loss;// for experiment
int heavy_loss_times; // for experiment
int pkt_num_input_thread,last_pkt_num_input_thread; // for experiment
// functions for actor statistic
struct actor_statistic * create_and_apply_actor_statistic(int *as_num){
    // monitor the actors
    struct actor_statistic *as_list = (struct actor_statistic *)malloc(SCHEDULER_MAX_ACTOR_NUM * sizeof(struct actor_statistic));
    *as_num = 0;
    struct subtask *st = NULL;
    struct thread_msg_req req;
    struct thread_msg_rsp *rsp;
    uint64_t freq = rte_get_timer_hz();

    req.type = THREAD_REQ_THREAD_MONITOR;
    int ***all_losses=(int ***)malloc(sizeof(int **)*MAX_CORE_NUM);
    all_losses[0] = (int **)malloc(sizeof(int *)*MAX_TASK_NUM+1);
    for(int j=0;j<MAX_TASK_NUM+1;j++){
        all_losses[0][j] = (int *)malloc(sizeof(int)*MAX_MULTI_PIPELINE_NUM);
        for(int k=0;k<MAX_MULTI_PIPELINE_NUM;k++){
            all_losses[0][j][k] = 0;
        }
    }
    for(int i=2+THREAD_INPUT_THREAD_NUM;i<MAX_CORE_NUM;i++){
        all_losses[i] = (int **)malloc(sizeof(int *)*MAX_TASK_NUM+1);
        for(int j=0;j<MAX_TASK_NUM+1;j++){
            all_losses[i][j] = (int *)malloc(sizeof(int)*MAX_MULTI_PIPELINE_NUM);
            for(int k=0;k<MAX_MULTI_PIPELINE_NUM;k++){
                all_losses[i][j][k] = 0;
            }
        }
        req.losses = all_losses[i];
        rsp = thread_msg_send_recv(i, &req);
        if(rsp->status != 0){
            printf("schedule - thread_id: %d get losses failed.\n", i);
        }else{
            for(int j=0;j<MAX_TASK_NUM+1;j++){
                for(int k=0;k<MAX_MULTI_PIPELINE_NUM;k++){
                    all_losses[0][j][k] += all_losses[i][j][k];
                }
            }
        }
    }

    while(st=get_next_subtask(st)){
        if(st->is_retired){
            continue;
        }
        // 获得周期内时钟周期消耗
        req.type = THREAD_REQ_ACTOR_MONITOR;
        for(int i = 0; i < st->actor_num; i++){
            req.at = st->actors[i];
            rsp = thread_msg_send_recv(st->actors[i]->thread_id, &req);
            if(rsp->status == 0){
                as_list[*as_num].task_id = st->task_id;
                as_list[*as_num].actor_id = st->actors[i]->actor_id;
                as_list[*as_num].global_actor_id = st->actors[i]->global_actor_id;
                as_list[*as_num].thread_id = st->actors[i]->thread_id;
                as_list[*as_num].cycles = rsp->monitor_rsp.stat.cycles_cost_per_duration;
                // as_list[*as_num].cycles_dispatch = rsp->monitor_rsp.stat.cycles_cost_dispatch_per_duration;
                as_list[*as_num].read_packets = rsp->monitor_rsp.stat.read_packets_per_duration;
                as_list[*as_num].num_0 = rsp->monitor_rsp.stat.num_0;
                as_list[*as_num].num_1 = rsp->monitor_rsp.stat.num_1;
                as_list[*as_num].num_2 = rsp->monitor_rsp.stat.num_2;
                as_list[*as_num].loss = rsp->monitor_rsp.stat.loss;
                as_list[*as_num].recv_loss = all_losses[0][st->task_id][st->actors[i]->actor_id];
                as_list[*as_num].is_retired = rsp->monitor_rsp.is_retired;
                as_list[*as_num].is_exclusive = rsp->monitor_rsp.is_exclusive;
                as_list[*as_num].load = (float)rsp->monitor_rsp.stat.cycles_cost_per_duration/freq/SCHEDULE_SLEEP_TIME;
                as_list[*as_num].pressure = 1 - (float)rsp->monitor_rsp.stat.num_0 / (rsp->monitor_rsp.stat.num_0 + rsp->monitor_rsp.stat.num_1 + rsp->monitor_rsp.stat.num_2);
                st->actors[i]->load = as_list[*as_num].load;
                *as_num+=1;
            }
            thread_msg_free(rsp);
        }
        
    }
    last_pkt_num_input_thread = pkt_num_input_thread;
    pkt_num_input_thread = 0;
    req.type = THREAD_REQ_INPUT_MONITOR;
    for(int i=2;i<2+THREAD_INPUT_THREAD_NUM;i++){
        rsp = thread_msg_send_recv(i, &req);
        if(rsp->status == 0){
            uint64_t freq = rte_get_timer_hz();
            #ifdef DEBUG
            printf("schedule - thread_id: %d pkt_num: %d, cycles/feq: %f, dispatch loss: %d\n", i,rsp->monitor_rsp.stat.read_packets_per_duration, (double)rsp->monitor_rsp.stat.cycles_cost_per_duration/freq/SCHEDULE_SLEEP_TIME, rsp->monitor_rsp.stat.loss);
            #endif
            pkt_num_input_thread += rsp->monitor_rsp.stat.read_packets_per_duration;
            //将实验1.3.X运行raw结果写入experiment/1.3/1.3.X/out
            #ifdef DEBUG_1_3_X
            FILE *file = fopen("/home/hjjiang/hjjiang_capture_v3/node/experiment/1.3/1.3.1/out", "a");
            if (file == NULL) {
                perror("Failed to open file");
                exit(EXIT_FAILURE);
            }
            fprintf(file, "%d %d %d %d %d\n", rsp->monitor_rsp.stat.filter_num, rsp->monitor_rsp.stat.match_count, rsp->monitor_rsp.stat.total_match_count, rsp->monitor_rsp.stat.pkt_count, rsp->monitor_rsp.stat.total_pkt_count);
            fclose(file);
            #endif

        }
        if(rsp->monitor_rsp.stat.read_packets_per_duration==0){
            times = 0;
            heavy_loss_times = 0;
            flag = false;
        }
        thread_msg_free(rsp);
    }
    
    struct rte_eth_stats stats;
    // 获取端口的统计信息
    if (rte_eth_stats_get(0, &stats) == 0) {
        uint64_t dropped_packets = stats.imissed; // 获取丢弃的接收包数量
        float loss_rate = (float)(dropped_packets-last_loss)/pkt_num_input_thread;
        #ifdef DEBUG
        printf("schedule - port %d's packet loss when receiving: %" PRIu64 " %f\n", 0, dropped_packets,loss_rate);
        #endif
        // for(int i=0;i<THREAD_INPUT_THREAD_NUM;i++){
        //     printf("schedule - 队列 %d 成功接收数据包总数: %d\n", stats.q_ipackets);
        //     printf("schedule - 队列 %d 丢弃数据包总数    : %d\n", stats.q_errors);
        // }
        
        if(loss_rate >= 0.001){
            heavy_loss_times++;
            if(heavy_loss_times==3){
                printf("schedule - Packet loss rate exceeded 0.001 for three consecutive times\n");
                heavy_loss_times = 0;
                flag = 1;
            }
        }else{
            heavy_loss_times = 0;
        }
        times++;
        if(times == 15){
            times = 0;
            heavy_loss_times = 0;
            #ifdef DEBUG
            printf("schedule - Flag measured consecutively 15 times: %d\n", flag);
            #endif
            flag = false;
        }
        last_loss = dropped_packets;
    } else {
        #ifdef DEBUG
        printf("schedule - Failed to get statistics for port %d.\n", 0);
        #endif
    }
    
    
    

    return as_list;
}

void free_actor_statistic(struct actor_statistic *as_list){
    free(as_list);
}

void get_task_load_and_pressure(struct actor_statistic *as_list, int as_num,int task_id, int *actor_num, float *pressure, float *load, float *min_load, float *loss_rate){
    int num = 0;
    float max_p = 0, max_l = 0, max_loss_rate = 0, min_l=1;
    for(int i = 0; i < as_num; i++){
        if(!as_list[i].is_retired && as_list[i].task_id == task_id){
            if(as_list[i].pressure>max_p){
                max_p = as_list[i].pressure;
            }
            if(as_list[i].load>max_l){
                max_l = as_list[i].load;
            }
            if(as_list[i].load<min_l){
                min_l = as_list[i].load;
            }
            float loss_rate = (float)as_list[i].recv_loss/as_list[i].read_packets;
            if(max_loss_rate < loss_rate){
                max_loss_rate = loss_rate;
            }
            num++;
        }
    }
    *actor_num = num;
    *pressure = max_p;
    *load = max_l;
    *min_load = min_l;
    *loss_rate = max_loss_rate;
}

void get_thread_load_and_pressure(struct actor_statistic *as_list, int as_num,int thread_id, int *actor_num, float *pressure, float *load){
    int num = 0;
    float max_p = 0, sum_l = 0;
    for(int i = 0; i < as_num; i++){
        if(!as_list[i].is_retired && as_list[i].thread_id == thread_id){
            if(as_list[i].pressure>max_p){
                max_p = as_list[i].pressure;
            }
            sum_l += as_list[i].load;
            num++;
        }
    }
    *actor_num = num;
    *pressure = max_p;
    *load = sum_l;
}

int get_max_ring_len(struct actor_statistic *as_list, int as_num, int task_id){
    int max_ring_len = 0;
    for(int i = 0; i < as_num; i++){
        if(!as_list[i].is_retired && as_list[i].task_id == task_id){
            if(as_list[i].max_ring_len > max_ring_len){
                max_ring_len = as_list[i].max_ring_len;
            }
        }
    }
    return max_ring_len;
}

void update_task_object_difference(){
    struct subtask *st = NULL;
    while(st=get_next_subtask(st)){
        for(int i=0;i<st->obj_num;i++){
            InterObject *ori_obj = st->objs[i];
            for(int k=0;k<MAX_INTERSECTION_NUM;k++){
                InterObject *alpha_1 = intersection_object(k);
                if(alpha_1==NULL){
                    continue;
                }
                if(contain_intersection_object_or_not(ori_obj, alpha_1)){
                    DifferObject *d_alpha = find_difference_object(k);
                    st->d[st->d_num++] = d_alpha;
                }
            }
        }
    }
}
// decrease one actor when st->actor_num > 1
int decrease_one_actor(struct subtask *st){
    pthread_mutex_lock(&st->lock);
    struct actor *actor = st->actors[st->actor_num - 1];
	pthread_mutex_unlock(&st->lock);
    undeploy_actor(actor);
    actor_disable(actor);
    actor_free(actor);
    return 0;
}

//求大于等于n的最小的2的幂
unsigned int next_power_of_two(unsigned int n) {
    if (n == 0) return 1; // 0 的下一个 2 的幂是 1
    n--; // 先减去 1，以便处理边界情况
    n |= n >> 1; // 将最高位的 1 移动到低位
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16; // 针对 32 位整数
    return n + 1; // 返回下一个 2 的幂
}

// increase one actor
int increase_one_actor(struct subtask *st, int max_ring_len){
    int thread_id = get_available_thread_id();
    if(thread_id == -1){
        printf("schedule - get_available_thread_id failed\n");
        return -1;
    }
    struct actor_params at_params;
    at_params.task_id = st->task_id;
    at_params.run_actor_times_per_manage = st->run_actor_times_per_manage;
    at_params.ring_size = next_power_of_two(max_ring_len);
    at_params.res_ring_size = st->res_ring_size;
    at_params.plugin_num = st->pi_num;
    for(int j = 0; j < st->pi_num; j++){
        at_params.plugin_params[j] = st->pis[j];
    }

    struct actor *actor = actor_create(&at_params);
    if(actor == NULL){
        printf("schedule - actor create failed\n");
        return -1;
    }
    actor_enable(st, actor);
    actor->thread_id = thread_id;
    pthread_mutex_lock(&actor->lock);
    actor->is_exclusive = true;
    pthread_mutex_unlock(&actor->lock);
    deploy_actor(actor);
    return 0;
}

// functions for scheduling
bool condition_satisfy(struct subtask_params* param) {
    time_t current_time = time(NULL);
    int seconds = (int)current_time;
    return param->retry_time <= seconds;
}
bool condition_retired(struct subtask_params* param) {
    return param->retrycount >= SCHEDULE_MAX_K;
}


int sche_InitialScheduling(void *arg) {
    #ifdef DEBUG
    printf("schedule - Initial Scheduling Thread Started\n");
    #endif
    TaskList = (struct task_queue *)malloc(sizeof(struct task_queue));
    init_task_queue(TaskList);
    wait_queue = (struct task_queue *)malloc(sizeof(struct task_queue));
    init_task_queue(wait_queue);
    while (true) {
        // sleep(1); // Wait for 5 seconds
        struct subtask_params t = dequeue_subtask_params(TaskList);
        if(strcmp(t.name, "") != 0){
            once_InitialScheduling(&t, wait_queue);
        }
        
        struct subtask_params t1;
        while(true){
            t1 = dequeue_subtask_params_with_condition(wait_queue, &condition_retired);
            if(strcmp(t1.name, "") == 0){
                break;
            }
        };

        struct subtask_params t2;
        while(true){
            t2 = dequeue_subtask_params_with_condition(wait_queue, &condition_satisfy);
            if(strcmp(t2.name, "") == 0){
                break;
            }
            once_InitialScheduling(&t2, wait_queue);
            // sleep(0.5);
        };
    }
}

void once_InitialScheduling(struct subtask_params *t, struct task_queue * wait_queue){
    int r = t->cpu_num;
    pthread_mutex_lock(&subtask_lock);
    if (count_get_available_threads() >= r) {
        #ifdef DEBUG6_1
        struct timeval start, end;
        // 获取开始时间
        gettimeofday(&start, NULL);
        #endif
        // init the task
        
        struct subtask *subtask = subtask_create(t);
        struct actor_params at_params;
        at_params.task_id = subtask->task_id;
        at_params.run_actor_times_per_manage = subtask->run_actor_times_per_manage;
        at_params.res_ring_size = subtask->res_ring_size;
        at_params.ring_size = NUM_MBUFS_DEFAULT;
        at_params.plugin_num = subtask->pi_num;
        for(int j = 0; j < subtask->pi_num; j++){
            at_params.plugin_params[j] = subtask->pis[j];
        }
        for(int i = 0;i<subtask->cpu;i++){
            struct actor *actor = actor_create(&at_params);
            if(actor == NULL){
                printf("schedule - actor create failed\n");
                return;
            }
            actor_enable(subtask, actor);

            int thread_id = get_available_thread_id();
            if(thread_id == -1){
                printf("schedule - get_available_thread_id failed\n"); 
                exit(1);
            }
            printf("schedule - task[%d] actor[%d] thread_id: %d\n", subtask->task_id,actor->actor_id,thread_id);
            actor->thread_id = thread_id;
            pthread_mutex_lock(&actor->lock);
            actor->is_exclusive = true;
            pthread_mutex_unlock(&actor->lock);
        }
        set_thread_all_ready();
        // calculate the number of difference objects
        int d_num = add_task_all_CS(subtask->task_id, subtask->objs, subtask->obj_num, subtask->d);

        subtask->d_num = d_num;
        
        deploy_subtask(subtask);
        int **A=produce_global_A();
        for(int i=0;i<MAX_INTERSECTION_NUM;i++){
            if(A[i][MAX_TASK_NUM]==-1){
                continue;
            }
            #ifdef DEBUG
            printf("schedule - object [%d]: ",i);
            #endif
            int first = A[i][MAX_TASK_NUM];
            for(int j=0;j<A[i][MAX_TASK_NUM+1];j++){
                #ifdef DEBUG
                printf("%d ",first);
                #endif
                first = A[i][first];
            }
            #ifdef DEBUG
            printf("\n");
            #endif
        }
        task_register(A);
        for(int i=0;i<MAX_INTERSECTION_NUM;i++){
            free(A[i]);
        }
        free(A);
        // update the filter in input thread
        
        update_filter_group();

        #ifdef DEBUG6_1
        // 获取结束时间
        gettimeofday(&end, NULL);

        // 计算时间差
        long seconds = end.tv_sec - start.tv_sec;
        long microseconds = end.tv_usec - start.tv_usec;

        printf("时间差: %ld 微秒\n", microseconds);
        #endif
    } else {
        printf("schedule - Not enough threads available\n");
        t->retrycount++;
        time_t current_time = time(NULL);
        int seconds = (int)current_time;
        t->retry_time = SCHEDULE_WAIT_TIME + seconds;
        enqueue_subtask_params(wait_queue, *t); // Re-enqueue the task
    }
    pthread_mutex_unlock(&subtask_lock);
}

int sche_rescheduling(void *arg) {
    #ifdef DEBUG
    printf("schedule - Rescheduling Thread Started\n");
    #endif
    uint64_t time;
    uint64_t freq;
    struct actor_statistic *as_list;
    struct subtask *st = NULL;
    int l_used_thread=-1, used_thread=-1;
    int run_schedule_cycle = 0;
    int last_stable_cycle = 0;
    bool flag = false;
    while(true){
        // Wait for a certain amount of time
        sleep(SCHEDULE_SLEEP_TIME); // Wait for 5 seconds
        time = 0;
        freq = rte_get_timer_hz();
        int thread_is_added[MAX_CORE_NUM];
        int thread_is_decreased[MAX_CORE_NUM];
        for(int i = 0; i < MAX_CORE_NUM; i++){
            thread_is_added[i] = 0;
            thread_is_decreased[i] = 0;
        }
        int as_num;
        pthread_mutex_lock(&subtask_lock);
        as_list = create_and_apply_actor_statistic(&as_num);
        
        int used_thread_ids[MAX_CORE_NUM];
        for(int i = 0; i < MAX_CORE_NUM; i++){
            used_thread_ids[i] = 0;
        }
        for(int i = 0; i < as_num; i++){
            #ifdef DEBUG
            printf("schedule - %d: task_id: %2d, actor_id: %2d, global_actor_id: %d,thread_id: %2d, pkt_num: %7d, cycles/feq: %1.4f, is_retired: %1d, num[%9d %7d %7d %2.4f], loss: %d, recv_loss: %d\n", i,as_list[i].task_id, as_list[i].actor_id, as_list[i].global_actor_id,as_list[i].thread_id, as_list[i].read_packets, (double)as_list[i].cycles/freq/SCHEDULE_SLEEP_TIME,as_list[i].is_retired, as_list[i].num_0, as_list[i].num_1, as_list[i].num_2,1-(double)as_list[i].num_0/ (as_list[i].num_0 + as_list[i].num_1 + as_list[i].num_2), as_list[i].loss, as_list[i].recv_loss);
            #endif
            used_thread_ids[as_list[i].thread_id] = 1;
        }
        int used_thread_count = 0;
        for(int i = 0; i < MAX_CORE_NUM; i++){
            if(used_thread_ids[i] == 1){
                used_thread_count++;
            }
        }
        printf("schedule - Used thread count: %d\n", used_thread_count);
        
        // float pkt_vary_ratio = (float)last_pkt_num_input_thread/(pkt_num_input_thread+1)>1?(float)last_pkt_num_input_thread/(pkt_num_input_thread+1)-1:1-(float)last_pkt_num_input_thread/(pkt_num_input_thread+1);
        // #ifdef DEBUG_8_1
        // printf("schedule - pkt_num_input_thread: %d, last_pkt_num_input_thread: %d, pkt_vary_ratio: %f\n", pkt_num_input_thread, last_pkt_num_input_thread, pkt_vary_ratio);
        // #endif
        // if(flag && used_thread_count == used_thread && used_thread_count == l_used_thread){
        //     #ifdef DEBUG_8_1
        //     printf("schedule - system comes stable status\n");
        //     FILE *file = fopen("/home/hjjiang/hjjiang_capture_v3/node/experiment/8.1/cost_schedule_cycles_05", "a");
        //     if (file == NULL) {
        //         perror("Failed to open file");
        //         exit(EXIT_FAILURE);
        //     }
        //     int delta = run_schedule_cycle - last_stable_cycle;
        //     fprintf(file, "%d %d %d %d\n", pkt_num_input_thread, run_schedule_cycle, last_stable_cycle, delta);
        //     printf("schedule - ---------system comes stable status, cycle: %d\n", run_schedule_cycle);
        //     fclose(file);
        //     flag = false;
        //     #endif
        // }
        // if(!flag&&pkt_vary_ratio > 0.05&&pkt_num_input_thread>2000){ 
        //     last_stable_cycle = run_schedule_cycle;
        //     #ifdef DEBUG_8_1
        //     printf("schedule - ---------system start unstable status\n");
        //     #endif
        //     flag = true;
        // }
        // run_schedule_cycle += 1;
        // l_used_thread = used_thread;
        // used_thread = used_thread_count;
        FILE *file = fopen("/home/hjjiang/hjjiang_capture_v3/node/experiment/8.1/cost_schedule_cycles_10", "a");
        if (file == NULL) {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%f %d\n", (float)pkt_num_input_thread/979200.0, used_thread_count);
        fclose(file);
        pthread_mutex_unlock(&subtask_lock);
        // rescheduling
        while(true){
            pthread_mutex_lock(&subtask_lock);
             
            st = get_earliest_subtask_after_time(time);
            
            if(st == NULL){
                pthread_mutex_unlock(&subtask_lock);
                break;
            }
            time = st->arrive_time;
            bool st_is_retired = true;
            for(int i = 0; i < as_num; i++){
                if(as_list[i].task_id == st->task_id){
                    if(!as_list[i].is_retired){
                        st_is_retired = false;
                        pthread_mutex_unlock(&subtask_lock);
                        break;
                    }
                }
            }
            if(st_is_retired){
                #ifdef DEBUG
                printf("schedule - task %d is retired\n", st->task_id);
                #endif
                // 更新S
                remove_task_all_CS(st->task_id, st->objs, st->obj_num);
                // 更新filter group
                update_filter_group();
                // 更新A
                int **A=produce_global_A();
                task_register(A);
                for(int i=0;i<MAX_INTERSECTION_NUM;i++){
                    free(A[i]);
                }
                free(A);
                // 更新B
                delete_B(st->task_id);
                undeploy_subtask(st);
                #ifdef DEBUG
                printf("schedule - task %d is undeployed\n", st->task_id);
                #endif
                for(int i = 0; i < st->actor_num; i++){
                    struct actor *actor = st->actors[i];
                    actor_disable(actor);
                    actor_free(actor);
                }
                #ifdef DEBUG
                printf("schedule - task %d's actors is undeployed\n", st->task_id);
                #endif
                subtask_free(st);

                pthread_mutex_unlock(&subtask_lock);
                continue;
            }
            // print_AB(st->actors[0]->thread_id);
            // 根据n0,n1,n2计算该任务的压力水平p=1-n_0/n
            float max_p,max_l,min_l,max_lossrate;
            int task_actor_num;
            get_task_load_and_pressure(as_list, as_num,st->task_id, &task_actor_num, &max_p, &max_l,&min_l,&max_lossrate);
            #ifdef DEBUG
            printf("schedule - task %d's max_p: %f, max_l: %f\n", st->task_id, max_p, max_l);
            #endif
            
            if(max_l<SCHEDULE_THETA_4&&max_p<SCHEDULE_THETA_1){ //&&max_lossrate<SCHEDULE_LOSS_THRESHOLD
                // 该任务是轻负载的
                // 该任务是轻负载的，但是actor数量大于1
                if(task_actor_num > 1){
                    #ifdef DEBUG
                    printf("schedule - -->task %d's light-load actor %d is removed\n",st->task_id, st->actors[st->actor_num-1]->actor_id);
                    #endif
                    if(max_l < SCHEDULE_THETA_4 - SCHEDULE_DOWN_TOLERANCE || st->last_load < SCHEDULE_THETA_4 ){
                        decrease_one_actor(st);
                    }
                }else{
                    st->actors[0]->is_exclusive = false;
                    if(st->actors[0]->cooldown > 0){
                        st->actors[0]->cooldown--;
                    }else{
                        int old_thread_id = st->actors[0]->thread_id;
                        float max_td_p,max_td_l;
                        int thread_actor_num;
                        get_thread_load_and_pressure(as_list, as_num,old_thread_id, &thread_actor_num, &max_td_p, &max_td_l);
                        if(thread_actor_num< SCHEDULE_MIN_STABLE_ACTOR_NUM && !thread_is_added[old_thread_id]){
                            uint32_t i;
                            float p=0, l=0;
                            int max_tgt_actor_num = 0;
                            int k=-1;
                            RTE_LCORE_FOREACH_WORKER(i){ // 将该actor迁移到具有最多任务且满足没有正常负载任务的线程上，设置其冷却时间为5 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
                                if(i < 2+THREAD_INPUT_THREAD_NUM || i>=MAX_CORE_NUM){
                                    continue;
                                }
                                float tgt_max_td_p,tgt_max_td_l;
                                int tgt_thread_actor_num;
                                get_thread_load_and_pressure(as_list, as_num,i, &tgt_thread_actor_num, &tgt_max_td_p, &tgt_max_td_l);
                                bool is_exclusive = false;
                                bool enabled = false;
                                int thread_actor_num;
                                get_thread_attributes(i, &enabled, &is_exclusive, &thread_actor_num);
                                if(thread_is_decreased[i] || thread_is_added[i] || !enabled || tgt_thread_actor_num> SCHEDULE_MAX_STABLE_ACTOR_NUM || is_exclusive || tgt_max_td_l > SCHEDULE_THETA_4){
                                    continue;
                                }
                                if(tgt_thread_actor_num > max_tgt_actor_num){
                                    max_tgt_actor_num = tgt_thread_actor_num;
                                    p = tgt_max_td_p;
                                    l = tgt_max_td_l;
                                    k = i;
                                }
                            }
                            if(k==-1){
                                // 无法找到合适的线程
                                continue;
                            }else if(k==old_thread_id){
                                continue;
                            }else{
                                struct actor *actor = st->actors[0];
                                actor->cooldown = SCHEDULE_COOLDOWN;
                                transfer_actor(actor, k);
                                thread_is_added[k] = 1;
                                thread_is_decreased[old_thread_id] = 1;
                                #ifdef DEBUG
                                printf("schedule - -->task %d's light-load actor %d is transmit from thread %d to thread %d\n", st->task_id,actor->actor_id, old_thread_id,k);
                                #endif
                            }
                        }
                    }
                }
            }else if(max_lossrate<SCHEDULE_LOSS_THRESHOLD&&(max_l<SCHEDULE_THETA_5&&max_p<SCHEDULE_THETA_1 || (max_l<SCHEDULE_THETA_3&&max_p<SCHEDULE_THETA_1&&st->actors[0]->is_exclusive))){
                // 该任务是正常负载的,回调机制
                // 有条件地减少actor数量1
                if(min_l<SCHEDULE_MIN_LOAD_IN_NORMAL_TASK && task_actor_num>1 &&min_l+max_l<SCHEDULE_THETA_3){
                    #ifdef DEBUG
                    printf("schedule - -->normal task %d's light-load actor %d is removed\n",st->task_id, st->actors[st->actor_num-1]->actor_id);
                    #endif
                    decrease_one_actor(st);
                }
            }else{
                // 该任务是重负载的
                if(max_l>=SCHEDULE_THETA_3+SCHEDULE_UP_TOLERANCE || st->last_load >= SCHEDULE_THETA_3 || max_p>=SCHEDULE_THETA_1 ){ //|| max_lossrate>=SCHEDULE_LOSS_THRESHOLD
                    int inc_num = floor(st->actor_num*0.5)>1?floor(st->actor_num*0.5):1;
                    if(count_get_available_threads() >= inc_num + st->actor_num){
                        #ifdef DEBUG
                        printf("schedule - -->task %d's heavy-load actor %d is added with transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                        #endif
                        for(int i=0;i<inc_num;i++){
                            if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                printf("schedule - increase_one_actor failed\n");
                            }
                        }
                        //将该任务的所有actor迁移到单独的线程上
                        for(int i = 0; i < st->actor_num; i++){
                            struct actor *actor = st->actors[i];
                            if(actor->is_exclusive){
                                continue;
                            }
                            actor->cooldown = 0;
                            actor->is_exclusive = true;
                            transfer_actor(actor, get_available_thread_id());
                        }
                        
                    }else if (count_get_available_threads() >= inc_num){
                        #ifdef DEBUG
                        printf("schedule - -->task %d's heavy-load actor %d is added without transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                        #endif
                        for(int i=0;i<inc_num;i++){
                            if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                printf("schedule - increase_one_actor failed\n");
                            }
                        }
                        //不将该任务的所有actor迁移到单独的线程上
                        for(int i = 0; i < st->actor_num; i++){
                            struct actor *actor = st->actors[i];
                            if(actor->is_exclusive){
                                continue;
                            }
                            actor->cooldown = 0;
                        }
                        
                    }else{
                        #ifdef DEBUG
                        printf("schedule - -->task %d's heavy-load actor is not added\n", st->task_id);
                        #endif
                    }
                }
            }
            st->last_load = max_l;
            pthread_mutex_unlock(&subtask_lock);
        }
        set_thread_all_ready();
        free(as_list);
    }

}

// 给定负载和压力，返回任务负载类型
int task_load_type(float l, float p, bool exclusive){
    if(exclusive){
        if(l<SCHEDULE2_L_THETA_5 && p<SCHEDULE2_P_THETA_1){
            return 1;//超轻负载
        }else if(l<SCHEDULE2_L_THETA_4 && p<SCHEDULE2_P_THETA_1){
            return 2;
        }else if(l<SCHEDULE2_L_THETA_3 && p<SCHEDULE2_P_THETA_1){
            return 3;
        }else if(l<SCHEDULE2_L_THETA_2&& p<SCHEDULE2_P_THETA_1){
            return 4;
        }else{
            return 5;
        }
    }else{
        if(l<SCHEDULE2_L_THETA_5*SCHEDULE2_L_THETA_SCALE && p<SCHEDULE2_P_THETA_1){
            return 1;//超轻负载
        }else if(l<SCHEDULE2_L_THETA_4*SCHEDULE2_L_THETA_SCALE && p<SCHEDULE2_P_THETA_1){
            return 2;
        }else if(l<SCHEDULE2_L_THETA_3*SCHEDULE2_L_THETA_SCALE && p<SCHEDULE2_P_THETA_1){
            return 3;
        }else if(l<SCHEDULE2_L_THETA_2*SCHEDULE2_L_THETA_SCALE && p<SCHEDULE2_P_THETA_1){
            return 4;
        }else{
            return 5;
        }
    }
}

void mark_actor_light_load(struct actor *at, struct actor *light_ats[], int *num){
    at->is_exclusive = false;
    light_ats[*num] = at;
    *num += 1;
}

int reset_thread_exclusive_status(int thread_id, bool is_exclusive){
    struct thread_msg_req req;
	int status;
	/* Write request */
	req.type = THREAD_REQ_THREAD_EXCLUSIVE_TRANS;
    req.is_exclusive = is_exclusive;
	/* Send request and wait for response */
	struct thread_msg_rsp *rsp = thread_msg_send_recv(thread_id, &req);
	thread_msg_free(rsp);
	/* Request completion */
	return 0;
}

int sche_rescheduling_2(void *arg) {
    #ifdef DEBUG
    printf("schedule - Rescheduling Thread Started\n");
    #endif
    uint64_t time;
    uint64_t freq;
    struct actor_statistic *as_list;
    struct subtask *st = NULL;
    int l_used_thread=-1, used_thread=-1;
    int run_schedule_cycle = 0;
    int last_stable_cycle = 0;
    bool flag = false;
    while(true){
        // Wait for a certain amount of time
        sleep(SCHEDULE_SLEEP_TIME); // Wait for SCHEDULE_SLEEP_TIME seconds
        time = 0;
        freq = rte_get_timer_hz();
        
        int as_num;
        pthread_mutex_lock(&subtask_lock);
        as_list = create_and_apply_actor_statistic(&as_num);
        
        int used_thread_ids[MAX_CORE_NUM];
        for(int i = 0; i < MAX_CORE_NUM; i++){
            used_thread_ids[i] = 0;
        }
        for(int i = 0; i < as_num; i++){
            #ifdef DEBUG
            printf("schedule - %2d: task_id: %2d, actor_id: %2d, global_actor_id: %2d,thread_id: %2d, pkt_num: %7d, cycles/feq: %1.4f, is_retired: %1d num_0_1_2[%9d %9d %9d %2.4f], loss: %d, recv_loss: %d\n", i,as_list[i].task_id, as_list[i].actor_id, as_list[i].global_actor_id,as_list[i].thread_id, as_list[i].read_packets, (double)as_list[i].cycles/freq/SCHEDULE_SLEEP_TIME,as_list[i].is_retired, as_list[i].num_0, as_list[i].num_1, as_list[i].num_2,1-(double)as_list[i].num_0/ (as_list[i].num_0 + as_list[i].num_1 + as_list[i].num_2), as_list[i].loss, as_list[i].recv_loss);
            #endif
            used_thread_ids[as_list[i].thread_id] = 1;
        }
        int used_thread_count = 0;
        for(int i = 0; i < MAX_CORE_NUM; i++){
            if(used_thread_ids[i] == 1){
                used_thread_count++;
            }
        }
        printf("schedule - Used thread count: %d\n", used_thread_count);
        #ifdef DEBUG_4_1
        FILE *file = fopen("/home/hjjiang/hjjiang_capture_v3/node/experiment/4.1/out", "a");
        if (file == NULL) {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < as_num; i++) {
            fprintf(file, "%d %d %d %f\n", as_list[i].task_id, as_list[i].actor_id, as_list[i].thread_id, (double)as_list[i].cycles / freq / SCHEDULE_SLEEP_TIME);
        }
        fprintf(file,"\n");
        fclose(file);
        #endif
        #ifdef DEBUG_8_1
        FILE *file = fopen("/home/hjjiang/hjjiang_capture_v3/node/experiment/8.1/used_threads_per_schedule_cycle_1G", "a");
        if (file == NULL) {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%f %d\n", (float)pkt_num_input_thread/676959.0, used_thread_count);
        fclose(file);
        #endif
        pthread_mutex_unlock(&subtask_lock);
        // rescheduling - increase/decrease actor, or mark actor as migratable
        struct actor *migratable_actors[as_num];
        int migratable_actor_num = 0;
        int cannot_migrate_thread[MAX_CORE_NUM];
        for(int i = 0; i < MAX_CORE_NUM; i++){
            cannot_migrate_thread[i] = 0;
        }
        while(true){
            pthread_mutex_lock(&subtask_lock);
            
            st = get_earliest_subtask_after_time(time);
            
            if(st == NULL){
                pthread_mutex_unlock(&subtask_lock);
                break;
            }
            time = st->arrive_time;
            bool st_is_retired = true;
            for(int i = 0; i < as_num; i++){
                if(as_list[i].task_id == st->task_id){
                    if(!as_list[i].is_retired){
                        st_is_retired = false;
                        pthread_mutex_unlock(&subtask_lock);
                        break;
                    }
                }
            }
            if(st_is_retired){
                #ifdef DEBUG
                printf("schedule - task %d is retired\n", st->task_id);
                #endif
                // 更新S
                remove_task_all_CS(st->task_id, st->objs, st->obj_num);
                // 更新filter group
                update_filter_group();
                // 更新A
                int **A=produce_global_A();
                task_register(A);
                for(int i=0;i<MAX_INTERSECTION_NUM;i++){
                    free(A[i]);
                }
                free(A);
                // 更新B
                delete_B(st->task_id);
                undeploy_subtask(st);
                #ifdef DEBUG
                printf("schedule - task %d is undeployed\n", st->task_id);
                #endif
                for(int i = 0; i < st->actor_num; i++){
                    struct actor *actor = st->actors[i];
                    actor_disable(actor);
                    actor_free(actor);
                }
                #ifdef DEBUG
                printf("schedule - task %d's actors is undeployed\n", st->task_id);
                #endif
                subtask_free(st);

                pthread_mutex_unlock(&subtask_lock);
                continue;
            }
            // print_AB(st->actors[0]->thread_id);
            // 根据n0,n1,n2计算该任务的压力水平p=1-n_0/n
            float max_p,max_l,min_l,max_lossrate;
            int task_actor_num;
            get_task_load_and_pressure(as_list, as_num,st->task_id, &task_actor_num, &max_p, &max_l,&min_l,&max_lossrate);
            #ifdef DEBUG
            printf("schedule - task %d's max_p: %f, max_l: %f\n", st->task_id, max_p, max_l);
            #endif
            
            int load_type = task_load_type(max_l, max_p, st->actors[0]->is_exclusive);
            //任务负载类型判断
            int inc_num;
            switch (load_type){
                case 1:
                    if(task_actor_num>1){
                        #ifdef DEBUG
                        printf("schedule - -->type: %d - task %d's light-load actor %d is removed\n",load_type,st->task_id, st->actors[st->actor_num-1]->actor_id);
                        #endif
                        decrease_one_actor(st);
                    }else{
                        mark_actor_light_load(st->actors[0], migratable_actors, &migratable_actor_num);
                    }
                    break;
                case 2:
                    switch(st->last_load_type){
                        case 1:
                        case 2:
                            if(task_actor_num>1){
                                #ifdef DEBUG
                                printf("schedule - -->type: %d - task %d's light-load actor %d is removed\n",load_type,st->task_id, st->actors[st->actor_num-1]->actor_id);
                                #endif
                                decrease_one_actor(st);
                            }else{
                                mark_actor_light_load(st->actors[0], migratable_actors, &migratable_actor_num);
                            }
                            break;
                        case 3:
                        case 4:
                        case 5:
                            for(int i=0;i<as_num;i++){
                                if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                    if(task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) <=2){
                                        struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                        mark_actor_light_load(temp_actor, migratable_actors, &migratable_actor_num);
                                    }
                                }
                            }
                            break;
                        case -1:
                            break;
                    }
                    break;
                case 3:
                    switch(st->last_load_type){
                        case 1:
                        case 2:
                            //转移其正常负载actor至单独空闲线程上
                            for(int i = 0; i < as_num; i++){
                                if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                    if(!as_list[i].is_exclusive&&task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) ==3){
                                        struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                        temp_actor->is_exclusive = true;
                                        int thread_actor_num;
                                        float max_l,max_p;
                                        get_thread_load_and_pressure(as_list, as_num,temp_actor->thread_id, &thread_actor_num, &max_l, &max_p);
                                        if(thread_actor_num==1){
                                            reset_thread_exclusive_status(temp_actor->thread_id, true);
                                        }else{
                                            transfer_actor(temp_actor, get_available_thread_id());
                                            cannot_migrate_thread[temp_actor->thread_id] = 1;
                                            #ifdef DEBUG
                                            printf("schedule - type: %d_%d - task %d's normal-load actor %d is transfered to thread %d\n",load_type,st->last_load_type,st->task_id, temp_actor->actor_id, temp_actor->thread_id);
                                            #endif
                                        }
                                        
                                    }
                                }
                            }
                            break;
                        case 3:
                            if(min_l+max_l<SCHEDULE2_L_THETA_3-0.5){
                                #ifdef DEBUG
                                printf("schedule - -->normal task %d's light-load actor %d is removed\n",st->task_id, st->actors[st->actor_num-1]->actor_id);
                                #endif
                                decrease_one_actor(st);
                            }else{
                                for(int i=0;i<as_num;i++){
                                    if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                        if(task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) <=2){
                                            struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                            mark_actor_light_load(temp_actor, migratable_actors, &migratable_actor_num);
                                        }
                                    }
                                }
                            }
                            break;
                        case 4:
                        case 5:
                            break;
                        case -1:
                            break;
                    }
                    break;
                case 4:
                    switch(st->last_load_type){
                        case 1:
                        case 2:
                            inc_num = floor(st->actor_num*0.5)>1?floor(st->actor_num*0.5):1;
                            if(count_get_available_threads() >= inc_num + st->actor_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added with transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                                //转移其正常负载actor至单独空闲线程上
                                for(int i = 0; i < as_num; i++){
                                    if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                        if(!as_list[i].is_exclusive&&task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) ==3){
                                            struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                            temp_actor->is_exclusive = true;
                                            int thread_actor_num;
                                            float max_l,max_p;
                                            get_thread_load_and_pressure(as_list, as_num,temp_actor->thread_id, &thread_actor_num, &max_l, &max_p);
                                            if(thread_actor_num==1){
                                                reset_thread_exclusive_status(temp_actor->thread_id, true);
                                            }else{
                                                transfer_actor(temp_actor, get_available_thread_id());
                                                cannot_migrate_thread[temp_actor->thread_id] = 1;
                                                #ifdef DEBUG
                                                printf("schedule - -->type: %d_%d - task %d's normal-load actor %d is transfered to thread %d\n",load_type,st->last_load_type,st->task_id, temp_actor->actor_id, temp_actor->thread_id);
                                                #endif
                                            }
                                            
                                        }
                                    }
                                }
                            }else if (count_get_available_threads() >= inc_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added without transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                            }
                            break;
                        case 3:
                            break;
                        case 4:
                        case 5:
                            inc_num = floor(st->actor_num*0.5)>1?floor(st->actor_num*0.5):1;
                            if(count_get_available_threads() >= inc_num + st->actor_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added with transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                                //转移其正常负载actor至单独空闲线程上
                                for(int i = 0; i < as_num; i++){
                                    if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                        if(!as_list[i].is_exclusive&&task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) >=3){
                                            struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                            temp_actor->is_exclusive = true;
                                            int thread_actor_num;
                                            float max_l,max_p;
                                            get_thread_load_and_pressure(as_list, as_num,temp_actor->thread_id, &thread_actor_num, &max_l, &max_p);
                                            if(thread_actor_num==1){
                                                reset_thread_exclusive_status(temp_actor->thread_id, true);
                                            }else{
                                                transfer_actor(temp_actor, get_available_thread_id());
                                                cannot_migrate_thread[temp_actor->thread_id] = 1;
                                                #ifdef DEBUG
                                                printf("schedule - -->type: %d_%d - task %d's normal-load actor %d is transfered to thread %d\n",load_type,st->last_load_type,st->task_id, temp_actor->actor_id, temp_actor->thread_id);
                                                #endif
                                            }
                                        }
                                    }
                                }
                            }else if (count_get_available_threads() >= inc_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added without transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                            }
                            break;
                        case -1:
                            break;
                    }
                    break;
                case 5:
                    switch(st->last_load_type){
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            inc_num = floor(st->actor_num*0.5)>1?floor(st->actor_num*0.5):1;
                            if(count_get_available_threads() >= inc_num + st->actor_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added with transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                                //转移其正常负载actor至单独空闲线程上
                                for(int i = 0; i < as_num; i++){
                                    if(as_list[i].task_id == st->task_id && !as_list[i].is_retired){
                                        if(!as_list[i].is_exclusive&&task_load_type(as_list[i].load, as_list[i].pressure, as_list[i].is_exclusive) >=3){
                                            struct actor *temp_actor = st->actors[as_list[i].actor_id]; 
                                            temp_actor->is_exclusive = true;
                                            int thread_actor_num;
                                            float max_l,max_p;
                                            get_thread_load_and_pressure(as_list, as_num,temp_actor->thread_id, &thread_actor_num, &max_l, &max_p);
                                            if(thread_actor_num==1){
                                                reset_thread_exclusive_status(temp_actor->thread_id, true);
                                            }else{
                                                transfer_actor(temp_actor, get_available_thread_id());
                                                cannot_migrate_thread[temp_actor->thread_id] = 1;
                                                #ifdef DEBUG
                                                printf("schedule - -->type: %d_%d - task %d's normal-load actor %d is transfered to thread %d\n",load_type,st->last_load_type,st->task_id, temp_actor->actor_id, temp_actor->thread_id);
                                                #endif
                                            }
                                            
                                        }
                                    }
                                }
                            }else if (count_get_available_threads() >= inc_num){
                                #ifdef DEBUG
                                printf("schedule - -->task %d's heavy-load actor %d is added without transmit\n", st->task_id,st->actors[st->actor_num-1]->actor_id);
                                #endif
                                for(int i=0;i<inc_num;i++){
                                    if(increase_one_actor(st, NUM_MBUFS_DEFAULT)==-1){
                                        printf("schedule - increase_one_actor failed\n");
                                    }
                                }
                            }
                            break;
                        case -1:
                            break;
                    }
                    break;
                default:
                    break;
                
            }
            
            //集中处理（超）轻负载actor的迁移，稳定的同时最小化线程数量
            //1.获取这些轻负载actor所在各线程上所有可调度轻负载actor数量以及所有actor负载之和
            int thread_light_actor_count[MAX_CORE_NUM] = {0};
            float thread_total_load[MAX_CORE_NUM] = {0.0};

            for (int i = 0; i < migratable_actor_num; i++) {
                struct actor *actor = migratable_actors[i];
                int thread_id = actor->thread_id;
                thread_light_actor_count[thread_id]++;
            }

            for (int i = 0; i < as_num; i++) {
                int thread_id = as_list[i].thread_id;
                thread_total_load[thread_id] += as_list[i].load;
            }
            //2.将可调度actor数量不为零的线程按照可调度轻负载actor数量和所有actor负载之和两个指标升序排序，已可调度轻负载actor数量为主要指标
            // Create an array of thread indices
            int thread_indices[MAX_CORE_NUM];
            int thread_count = 0;
            for (int i = 0; i < MAX_CORE_NUM; i++) {
                if (thread_light_actor_count[i] > 0) {
                    thread_indices[thread_count++] = i;
                }
            }

            // Sort the threads based on the number of light actors and total load
            for (int i = 0; i < thread_count - 1; i++) {
                for (int j = 0; j < thread_count - i - 1; j++) {
                    int thread_id1 = thread_indices[j];
                    int thread_id2 = thread_indices[j + 1];
                    if (thread_light_actor_count[thread_id1] > thread_light_actor_count[thread_id2] ||
                        (thread_light_actor_count[thread_id1] == thread_light_actor_count[thread_id2] &&
                        thread_total_load[thread_id1] > thread_total_load[thread_id2])) {
                        // Swap the thread indices
                        int temp = thread_indices[j];
                        thread_indices[j] = thread_indices[j + 1];
                        thread_indices[j + 1] = temp;
                    }
                }
            }

            //3.排序后，从第1个线程起，如果线程总负载小于SCHEDULE2_L_THETA_4，按照负载从大到小的顺序选择该线程上的可调度actor，尝试将其调度到最后一个线程上，如果该actor负载与最后一个线程总负载之和超过SCHEDULE2_L_THETA_3，则尝试倒数第二个线程，以此类推，直到找到一个合适的线程或者所有线程都不合适。
            int thread_is_added[MAX_CORE_NUM];
            int thread_is_decreased[MAX_CORE_NUM];
            for(int i = 0; i < MAX_CORE_NUM; i++){
                thread_is_added[i] = 0;
                thread_is_decreased[i] = 0;
            }
            for (int i = 0; i < thread_count; i++) {
                int src_thread_id = thread_indices[i];
                if(thread_is_added[src_thread_id]==1 || cannot_migrate_thread[src_thread_id] == 1) continue;
                if (thread_total_load[src_thread_id] < SCHEDULE2_L_THETA_4) {
                    // Collect light actors from the source thread
                    struct actor *light_actors[MAX_CORE_NUM];
                    int light_actor_count = 0;
                    for (int j = 0; j < migratable_actor_num; j++) {
                        if (migratable_actors[j]->thread_id == src_thread_id) {
                            light_actors[light_actor_count++] = migratable_actors[j];
                        }
                    }

                    // Sort light actors by load in descending order
                    for (int j = 0; j < light_actor_count - 1; j++) {
                        for (int k = 0; k < light_actor_count - j - 1; k++) {
                            if (light_actors[k]->load < light_actors[k + 1]->load) {
                                struct actor *temp = light_actors[k];
                                light_actors[k] = light_actors[k + 1];
                                light_actors[k + 1] = temp;
                            }
                        }
                    }

                    // Try to migrate light actors to other threads
                    for (int j = 0; j < light_actor_count; j++) {
                        struct actor *actor = light_actors[j];
                        for (int k = thread_count - 1; k > i; k--) {
                            int dst_thread_id = thread_indices[k];
                            if (thread_is_decreased[dst_thread_id]==1|| cannot_migrate_thread[dst_thread_id] == 1||thread_light_actor_count[dst_thread_id]>SCHEDULE_MAX_STABLE_ACTOR_NUM) continue;
                            if (thread_total_load[dst_thread_id] + actor->load < SCHEDULE2_L_THETA_3) {
                                reset_thread_exclusive_status(dst_thread_id,false);
                                transfer_actor(actor, dst_thread_id);
                                #ifdef DEBUG
                                printf("schedule - cannot migrate thread %d\n", cannot_migrate_thread[dst_thread_id]);
                                printf("schedule - -->task %d's light-load actor %d is transmit from thread %d to thread %d\n", st->task_id,actor->actor_id, src_thread_id, dst_thread_id);
                                #endif
                                thread_is_added[dst_thread_id] = 1;
                                thread_is_decreased[src_thread_id] = 1;
                                thread_total_load[dst_thread_id] += actor->load;
                                thread_light_actor_count[dst_thread_id]++;
                                thread_total_load[src_thread_id] -= actor->load;
                                thread_light_actor_count[src_thread_id]--;
                                break;
                            }
                        }
                    }
                }
            }
            
            

            st->last_load_type = load_type;
            pthread_mutex_unlock(&subtask_lock);
        }
        set_thread_all_ready();
        free(as_list);
    }

}


