#include "subtask.h"

int count_transfer_between_tasks[32][5][5] = {0,};
int get_count_transfer_between_tasks(int k, int i,int j){
    return count_transfer_between_tasks[k][i][j];
}

// 维护任务号
pthread_mutex_t task_lock; // 定义互斥锁
bool id_available[MAX_ID];

void init_task_id_system() {
    for (int i = 0; i < MAX_ID; i++) {
        id_available[i] = true;
    }
    pthread_mutex_init(&task_lock, NULL);
}
int get_available_task_id() {
    pthread_mutex_lock(&task_lock);
    for (int i = 0; i < MAX_ID; i++) {
        if (id_available[i]) {
            id_available[i] = false;
            pthread_mutex_unlock(&task_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&task_lock);
    return -1; // No available id found
}
void release_task_id(int id) {
    pthread_mutex_lock(&task_lock);
    if (id >= 0 && id < MAX_ID) {
        id_available[id] = true;
    }
    pthread_mutex_unlock(&task_lock);
}
// 维护global actor_id
pthread_mutex_t actor_id_lock; // 定义互斥锁
bool global_actor_id_available[SCHEDULER_MAX_ACTOR_NUM];
void init_actor_id_system() {
    for (int i = 0; i < SCHEDULER_MAX_ACTOR_NUM; i++) {
        global_actor_id_available[i] = true;
    }
    pthread_mutex_init(&actor_id_lock, NULL);
}
int get_available_actor_id() {
    pthread_mutex_lock(&actor_id_lock);
    for (int i = 0; i < SCHEDULER_MAX_ACTOR_NUM; i++) {
        if (global_actor_id_available[i]) {
            global_actor_id_available[i] = false;
            pthread_mutex_unlock(&actor_id_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&actor_id_lock);
    return -1; // No available id found
}
void release_actor_id(int id) {
    pthread_mutex_lock(&actor_id_lock);
    if (id >= 0 && id < SCHEDULER_MAX_ACTOR_NUM) {
        global_actor_id_available[id] = true;
    }
    pthread_mutex_unlock(&actor_id_lock);
}


//维护任务列表

static struct subtask_list subtask_list;
pthread_mutex_t subtask_lock; // 定义互斥锁

int subtask_init(void)
{
	TAILQ_INIT(&subtask_list);
    pthread_mutex_init(&subtask_lock, NULL);
	return 0;
}

struct subtask *
subtask_find(int task_id){
    struct subtask *subtask;
    TAILQ_FOREACH(subtask, &subtask_list, node)
        if (task_id == subtask->task_id)
            return subtask;
    return NULL;
}

struct subtask *get_next_subtask(struct subtask *st) {
    if (st == NULL)
        return TAILQ_FIRST(&subtask_list);
    return TAILQ_NEXT(st, node);
}

struct subtask *get_earliest_subtask_after_time(uint64_t time) {
    struct subtask *earliest_subtask = NULL;
    uint64_t earliest_time = ULONG_MAX;
    struct subtask *subtask;
    TAILQ_FOREACH(subtask, &subtask_list, node) {
        if (!subtask->is_retired && subtask->arrive_time > time && subtask->arrive_time < earliest_time) {
            earliest_time = subtask->arrive_time;
            earliest_subtask = subtask;
        }
    }
    return earliest_subtask;
}

struct subtask *
subtask_create( struct subtask_params *params){
    int task_id = get_available_task_id();
    if (task_id == -1){
        printf("subtask - Error: No available task id\n");
        return NULL;
    }
    
    struct subtask *subtask = (struct subtask *)rte_zmalloc_socket(
            NULL, sizeof(struct subtask),
			RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
    if (subtask == NULL) {
        return NULL;
    }
    strcpy(subtask->name, params->name);
    subtask->task_id = task_id;
    subtask->res_ring_size = params->res_ring_size;
    subtask->actor_num = 0;
    pthread_mutex_init(&subtask->lock, NULL);
    subtask->cpu = params->cpu_num;
    subtask->mem = params->mem_num;

    subtask->obj_num = params->obj_num;
    subtask->objs = (InterObject **)malloc(sizeof(InterObject *)*params->obj_num);
    for(int i = 0; i < params->obj_num; i++){
        subtask->objs[i] = (InterObject *)malloc(sizeof(InterObject));
        memcpy(subtask->objs[i], &params->objs[i], sizeof(InterObject));
    }
    subtask->d_num = 0;
    subtask->d = (DifferObject **)malloc(sizeof(DifferObject *)*MAX_DIFFERENCE_NUM_PER_TASK);
    strcpy(subtask->filename, params->filename);
    subtask->pi_num = params->pi_num;
    for (int i = 0; i < params->pi_num; i++){
        subtask->pis[i] = params->pis[i];
    }
    
    subtask->time = params->time;
    subtask->epoch = params->epoch;
    subtask->handle = NULL;
    subtask->run_actor_times_per_manage = params->run_actor_times_per_manage;
    uint64_t cpu_freq = rte_get_tsc_hz();
    subtask->arrive_time = rte_rdtsc();
    subtask->retired_time = subtask->arrive_time + subtask->time*cpu_freq;

    subtask->last_load = 0;
    subtask->last_load_type = -1;
    TAILQ_INSERT_TAIL(&subtask_list, subtask, node);
    #ifdef DEBUG
    printf("subtask - subtask_create: %s\n", subtask->name);
    #endif
    return subtask;
}

int subtask_free(struct subtask *subtask){
    int id = subtask->task_id;
    TAILQ_REMOVE(&subtask_list, subtask, node);
    rte_free(subtask);
    #ifdef DEBUG
    printf("subtask - subtask_free: %s\n", subtask->name);
    #endif
    release_task_id(id);
    return 0;
}

struct actor *
actor_find(int task_id, int actor_id){
    struct subtask *subtask = subtask_find(task_id);
    if (subtask == NULL)
        return NULL;
    struct actor *actor;
    pthread_mutex_lock(&subtask->lock);
    for (int i = 0; i < subtask->actor_num; i++){
        actor = subtask->actors[i];
        if (actor->actor_id == actor_id)
            pthread_mutex_unlock(&subtask->lock);
            return actor;
    }
    pthread_mutex_unlock(&subtask->lock);
    return NULL;
}

struct actor *
actor_create(struct actor_params *params){
    struct actor *at = ( struct actor *)malloc(sizeof(struct actor));
    if (at == NULL) {
        printf("subtask - Error: actor malloc failed\n");
        return NULL;
    }
    at->global_actor_id = get_available_actor_id();
    if (at->global_actor_id == -1){
        printf("subtask - Error: No available actor id\n");
        return NULL;
    }
    at->actor_id = 0;
    at->task_id = params->task_id;

    struct rte_ring *msgq_req, *msgq_rsp, *ring_in;
    char name[NAME_MAX];
    /* MSGQs */
    snprintf(name, sizeof(name), "ACTOR-%04x-MSGQ-REQ", at->global_actor_id);

    msgq_req = rte_ring_create(name,
        ACTOR_MSGQ_SIZE,
        SOCKET_ID_ANY,
        RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);

    if (msgq_req == NULL) {
        /* MSGQs */
        printf("subtask - Error: actor msgq_req create failed\n");
        return NULL;
    }

    snprintf(name, sizeof(name), "ACTOR-%04x-MSGQ-RSP", at->global_actor_id);

    msgq_rsp = rte_ring_create(name,
        ACTOR_MSGQ_SIZE,
        SOCKET_ID_ANY,
        RING_F_SP_ENQ | RING_F_MC_RTS_DEQ);

    if (msgq_rsp == NULL) {
        rte_ring_free(msgq_req);
        printf("subtask - Error: actor msgq_rsp create failed\n");
        return NULL;
    }
    at->msgq_req = msgq_req;
    at->msgq_rsp = msgq_rsp;

    snprintf(name, sizeof(name), "ACTOR-%04x-RING-IN", at->global_actor_id);
    // ring_in = rte_ring_create(name, 
    //     params->ring_size>1024?params->ring_size:1024, SOCKET_ID_ANY , 
    //     RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);


    ring_in = rte_ring_create(name, 
        1024*2, SOCKET_ID_ANY , 
        RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);
    if (ring_in == NULL) {
        rte_ring_free(msgq_req);
		rte_ring_free(msgq_rsp);
        printf("subtask - Error: actor ring_in create failed\n");
        return NULL;
    }
    at->ring_in = ring_in;
    at->p = (struct pipeline *)malloc(sizeof(struct pipeline));
    for(int i=0;i<MAX_PLUGIN_NUM_ONEPIPELINE;i++){
        at->p->plugins[i] = NULL;
    }
    at->p->plugin_num = 0;
    at->p->k_params = (kafka_params *)malloc(sizeof(kafka_params));
    at->p->k_params->res_ring_size = params->res_ring_size;
    // initial producer
    snprintf(at->p->k_params->topic, sizeof(at->p->k_params->topic), "ACTOR-%04x-TOPIC", at->global_actor_id);
    char errstr[512];
    at->p->k_params->rk = producer_init(BROKER,errstr,sizeof(errstr));
    at->p->stat.read_packets_per_duration = 0;
    at->p->stat.cycles_cost_per_duration = 0;
    // at->p->stat.cycles_cost_dispatch_per_duration = 0;
    at->p->stat.read_packets = 0;
    at->p->stat.num_0 = 0;
    at->p->stat.num_1 = 0;
    at->p->stat.num_2 = 0;
    for (int j = 0; j < params->plugin_num; j++) {
        struct plugin_params *plugin_params = &params->plugin_params[j];
        struct plugin *plugin = plugin_create(at->p, plugin_params);
        if (plugin == NULL) {
            rte_ring_free(msgq_req);
            rte_ring_free(msgq_rsp);
            rte_ring_free(ring_in);
            printf("subtask - Error: plugin create failed\n");
            return NULL;
        }
    }
    pthread_mutex_init(&at->lock, NULL);
    at->run_actor_times_per_manage = params->run_actor_times_per_manage;
    at->run_actor_times = 0;
    at->dispatch = inter_dispatch;

    

    at->is_retired = false;
    at->cooldown = 0;
    #ifdef STATISTICS1
    at->time_shot = (double *)malloc(MAX_STATISTICS1_NUM*sizeof(double));
    at->sta_index = 0;

    #endif
    #ifdef DEBUG
    printf("subtask - create ring in task[%d] actor[%d] %p\n",at->task_id,at->global_actor_id,at->ring_in);
    printf("subtask - actor_create: %s\n", at->task_name);
    #endif
    return at;
};
int actor_enable(struct subtask *subtask, 
                    struct actor *actor){
    pthread_mutex_lock(&subtask->lock);
    if (subtask->actor_num >= MAX_SUBTASK_CORE_NUM){
        pthread_mutex_unlock(&subtask->lock);
        return -1;
    }
    actor->actor_id = subtask->actor_num;
    subtask->actors[subtask->actor_num] = actor;
    actor->retired_time = subtask->retired_time;
    subtask->actor_num++;
    strcpy(actor->task_name,subtask->name);
    #ifdef DEBUG
    printf("subtask - actor_enable: %s\n", actor->task_name);
    #endif
    pthread_mutex_unlock(&subtask->lock);
    return 0;
}

int actor_disable(struct actor *actor){
    struct subtask *subtask = subtask_find(actor->task_id);
    if (subtask == NULL)
        return -1;
    pthread_mutex_lock(&subtask->lock);
    for (int i = 0; i < subtask->actor_num; i++){
        if (subtask->actors[i] == actor){
            for(int j=i;j<subtask->actor_num-1;j++){
                subtask->actors[j] = subtask->actors[j+1];
            }
            subtask->actors[subtask->actor_num-1] = NULL;
            subtask->actor_num--;
            pthread_mutex_unlock(&subtask->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&subtask->lock);
    #ifdef DEBUG
    printf("subtask - actor_disable: %s\n", actor->task_name);
    #endif
    return -1;
}

int actor_free(struct actor *actor){
    int id = actor->global_actor_id;
    
    rte_ring_free(actor->msgq_req);
    rte_ring_free(actor->msgq_rsp);
    rte_ring_free(actor->ring_in);
    for (int i = 0; i < actor->p->plugin_num; i++){
        plugin_free(actor->p, actor->p->plugins[i]);
    }

    #ifdef STATISTICS1
    free(actor->time_shot);
    #endif
    free(actor);
    release_actor_id(id);
    #ifdef DEBUG
    printf("subtask - actor_free: %s\n", actor->task_name);
    #endif
    return 0;
}

struct plugin *
plugin_create(struct pipeline *p, 
                            struct plugin_params *params){
    
    if (p->plugins[params->plugin_index] != NULL){
        printf("subtask - Error: plugin already exists\n");
        return NULL;
    }
    struct plugin *plugin = (struct plugin *)malloc(sizeof(struct plugin));
    
    // init args
    if(p->plugin_num == 0){
        for(int i=0;i<MAX_PLUGIN_ARGS_NUM;i++){
            p->to_next[i] = rte_zmalloc_socket(
                NULL, sizeof(void *), RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
        }
        // p->func
        char filename[128];
        sprintf(filename, "%s/%s",NODE_PLUGIN_DIR,
                    params->filename);
        plugin->handle = dlopen(filename, RTLD_LAZY);
        if (plugin->handle == NULL){
            printf("subtask - fail to dlopen %s\n",filename);
            return NULL;
        }
        void *init_func = dlsym(plugin->handle, "init_args");
        if (init_func == NULL)
            return NULL;
        ((void (*)(void *))init_func)(p->to_next);
        Init_ONE_RESULT init_one_res_func = (Init_ONE_RESULT)dlsym(plugin->handle, "allocate_one_result");
        if(init_one_res_func == NULL){
            printf("subtask - fail to dlsym func allocate_one_result\n");
            return NULL;
        }
        p->k_params->res_ring = initRing(p->k_params->res_ring_size,init_one_res_func);

    }else{
        plugin->handle = p->plugins[0]->handle;
    }
    void *func = dlsym(plugin->handle, params->funcname);
    if (func == NULL){
        printf("subtask - fail to dlsym func %s\n",params->funcname);
        return NULL;   
    }
    plugin->func = (PF)func;
    
    // p->res
    Byte ****res = (Byte ****)rte_zmalloc_socket(
            NULL, params->cnt_info.rownum * sizeof(Byte ***), RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
    for (int i = 0; i < params->cnt_info.rownum; i++){
        res[i] = (Byte ***)rte_zmalloc_socket(
            NULL, params->cnt_info.bucketnum * sizeof(Byte **), RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
        for (int j = 0; j < params->cnt_info.bucketnum; j++){
            res[i][j] = (Byte **)rte_zmalloc_socket(
                NULL, params->cnt_info.bucketsize * sizeof(Byte *), RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
            for (int k = 0; k < params->cnt_info.bucketsize; k++){
                res[i][j][k] = (Byte *)rte_zmalloc_socket(
                    NULL, params->cnt_info.countersize * sizeof(Byte),RTE_CACHE_LINE_SIZE, SOCKET_ID_ANY);
            }
        }
    }
    plugin->res = res;
    plugin->plugin_index = params->plugin_index;
    plugin->cnt_info = params->cnt_info;
    p->plugins[plugin->plugin_index] = plugin;
    p->plugin_num++;

    
    return plugin;
}

int plugin_free(struct pipeline *p, struct plugin *plugin){
    for (int i = 0; i < plugin->cnt_info.rownum; i++){
        for (int j = 0; j < plugin->cnt_info.bucketnum; j++){
            for (int k = 0; k < plugin->cnt_info.bucketsize; k++){
                rte_free(plugin->res[i][j][k]);
            }
            rte_free(plugin->res[i][j]);
        }
        rte_free(plugin->res[i]);
    }
    rte_free(plugin->res);
    p->plugins[plugin->plugin_index] = NULL;
    p->plugin_num--;
    if (p->plugin_num == 0){
        for(int i=0;i<MAX_PLUGIN_ARGS_NUM;i++){
            rte_free(p->to_next[i]);
        }
        dlclose(plugin->handle);
    }
    
    free(plugin);
    return 0;
}

int run_pipeline(struct pipeline *p,struct rte_mbuf *pkt){
    // Calculate the CPU cycles used
    uint64_t cycles = rte_rdtsc();
    struct plugin **plugins = p->plugins;
    int temp = plugins[0]->func(pkt, plugins[0]->res,plugins[0]->cnt_info.rownum,plugins[0]->cnt_info.bucketnum,plugins[0]->cnt_info.bucketsize,plugins[0]->cnt_info.countersize,p->to_next,p->k_params);
    while (temp != -1)
    {
        temp = plugins[temp]->func(pkt, plugins[temp]->res,plugins[temp]->cnt_info.rownum,plugins[temp]->cnt_info.bucketnum,plugins[temp]->cnt_info.bucketsize,plugins[temp]->cnt_info.countersize,p->to_next,p->k_params);
    }
    
    // Perform your calculations here
    cycles = rte_rdtsc() - cycles;
    return 0;
}

// transport the object intra or inter subtasks
// int A[MAX_SUBTASK_CORE_NUM][MAX_INTERSECTION_NUM][MAX_TASK_NUM+2];
int ***A;
// B_Array B[MAX_SUBTASK_CORE_NUM][MAX_TASK_NUM+1];
B_Array **B;
uint32_t calculate_hash(const uint32_t* src_addr, const uint32_t* dst_addr) {
    uint32_t hash = 0;
    // Calculate hash based on source address
    hash = rte_hash_crc(src_addr, sizeof(uint32_t), hash);
    // Calculate hash based on destination address
    hash = rte_hash_crc(dst_addr, sizeof(uint32_t), hash);
    return hash;
}
void copy_B_loss(int thread_id, int **loss){
    for(int i=0;i<MAX_TASK_NUM+1;i++){
        for(int j=0;j<MAX_MULTI_PIPELINE_NUM;j++){
            loss[i][j] = B[thread_id][i].loss[j];
            B[thread_id][i].loss[j] = 0;
        }
    }
}

void get_packet_addr(struct rte_mbuf* mbuf, uint32_t* src_addr, uint32_t* dst_addr) {
    struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(mbuf, struct rte_ipv4_hdr *, (sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr)));
    *src_addr = ipv4_hdr->src_addr;
    *dst_addr = ipv4_hdr->dst_addr;
}

int loss[MAX_CORE_NUM];
void initAB(){
    A = (int ***)malloc(MAX_CORE_NUM * sizeof(int **));
    for(int i=0;i<MAX_CORE_NUM;i++){
        A[i] = (int **)malloc(MAX_INTERSECTION_NUM * sizeof(int *));
        for(int j=0;j<MAX_INTERSECTION_NUM;j++){
            A[i][j] = (int *)malloc((MAX_TASK_NUM+2) * sizeof(int));
        }
    }
    B = (B_Array **)malloc(MAX_CORE_NUM * sizeof(B_Array *));
    for(int i=0;i<MAX_CORE_NUM;i++){
        B[i] = (B_Array *)malloc((MAX_TASK_NUM+1) * sizeof(B_Array));
    }
    for(int c=0;c<MAX_CORE_NUM;c++){
        for(int i=0;i<MAX_INTERSECTION_NUM;i++){
            for(int j=0;j<MAX_TASK_NUM+1;j++){
                A[c][i][j] = -1;
            }
            A[c][i][MAX_TASK_NUM+1] = 0;
        }
        // A[c][0][MAX_TASK_NUM+1] = 1;
        // A[c][0][MAX_TASK_NUM] = 0;
        for(int i=0;i<MAX_TASK_NUM;i++){
            for(int j=0;j<MAX_MULTI_PIPELINE_NUM;j++){
                B[c][i].cur = 0;
                B[c][i].rings[j] = NULL;

            }
        }
    }
    for(int i=0;i<MAX_CORE_NUM;i++){
        loss[i] = 0;
    }
    
}

void prune_A(int **newA){
    bool tasks[MAX_TASK_NUM] = {false,};
    struct subtask *subtask;
    TAILQ_FOREACH(subtask, &subtask_list, node)
        tasks[subtask->task_id] = true;
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        int pre = MAX_TASK_NUM;
        int j;
        while(true){    
            j = newA[i][pre];
            if(j==-1){
                break;
            }
            if(!tasks[j]){
                newA[i][pre] = newA[i][j];
                newA[i][j] = -1;
            }else{
                pre = j;
            }
        }
    }
}

void update_A(int cpu_id, int **newA){
    for(int i=0;i<MAX_INTERSECTION_NUM;i++){
        for(int j=0;j<MAX_TASK_NUM+2;j++){
            A[cpu_id][i][j] = newA[i][j];
        }
    }
}
void updateA_add(int cpu_id, int object_id, int new_task_id){
    A[cpu_id][object_id][A[cpu_id][object_id][MAX_TASK_NUM]] = new_task_id;
    A[cpu_id][object_id][MAX_TASK_NUM] = new_task_id;
    A[cpu_id][object_id][MAX_TASK_NUM+1]++;
}
void updateA_del(int cpu_id, int object_id,int task_id){
    int next_task_id = A[cpu_id][object_id][task_id];
    int pre_task_id = -1;
    for(int i=0;i<MAX_TASK_NUM;i++){
        if(A[cpu_id][object_id][i] == task_id){
            pre_task_id = i;
            break;
        }
    }
    if(pre_task_id == -1){
        return;
    }
    if(next_task_id == -1){
        A[cpu_id][object_id][pre_task_id] = -1;
        A[cpu_id][object_id][MAX_TASK_NUM] = pre_task_id;
        A[cpu_id][object_id][MAX_TASK_NUM+1]--;
        return;
    }
    A[cpu_id][object_id][pre_task_id] = next_task_id;
    A[cpu_id][object_id][task_id] = -1;
    A[cpu_id][object_id][MAX_TASK_NUM]--;
    return;
}
void updateA_edit(int cpu_id, int object_id,int task_id,int next_task_id){
    A[cpu_id][object_id][task_id] = next_task_id;
}
int readA(int cpu_id, int object_id,int task_id){
    return A[cpu_id][object_id][task_id];
}


void updateB_add(int cpu_id,int task_id,struct rte_ring *ring){
    B[cpu_id][task_id].rings[B[cpu_id][task_id].cur++] = ring;
}

int find_B_loss(int cpu_id,int task_id,int actor_id){
    int loss = B[cpu_id][task_id].loss[actor_id];
    B[cpu_id][task_id].loss[actor_id] = 0;
    return B[cpu_id][task_id].loss[actor_id];
}

int find_thread_loss(int cpu_id){
    int lloss = loss[cpu_id];
    loss[cpu_id] = 0;
    return lloss;
}

void print_AB(int thread_id){
    printf("subtask - print A:\n");
    for(int j=0;j<30;j++){
        for(int k=0;k<MAX_TASK_NUM+2;k++){
            if(k<MAX_TASK_NUM&&k>15) continue;
            printf("%d ",A[thread_id][j][k]);
        }
        printf("\n");
    }
    printf("subtask - print B:\n");
    for(int j=0;j<15;j++){
        printf("task[%d](%d): ",j,B[thread_id][j].cur);
        for(int i=0;i<B[thread_id][j].cur;i++){
            printf(" %x",B[thread_id][j].rings[i]);
        }
        printf("\n");
    }
}
void updateB_del(int cpu_id,int task_id, struct rte_ring *ring){ 
    for (int i = 0; i < B[cpu_id][task_id].cur; i++) {
        if (B[cpu_id][task_id].rings[i] == ring) {
            for(int j=i;j<B[cpu_id][task_id].cur-1;j++){
                B[cpu_id][task_id].rings[j] = B[cpu_id][task_id].rings[j+1];
            }
            B[cpu_id][task_id].cur -= 1;
            break;
        }
    }
}

void updateB_del_task(int cpu_id, int task_id){
    for (int i = 0; i < B[cpu_id][task_id].cur; i++) {
        B[cpu_id][task_id].rings[i] = NULL;
    }
    B[cpu_id][task_id].cur = 0;
}

void inter_dispatch(int thread_id,int task_id,MARKID *markid,struct rte_mbuf *pkt){
    int actor_id;
    int object_id = markid->id1.object_id;
    if(object_id == 65535){
        rte_pktmbuf_free(pkt);
        return;
    }
    struct rte_ring *ring=NULL;
    int next_task_id = A[thread_id][object_id][task_id];
    if(next_task_id == -1){
        rte_pktmbuf_free(pkt);
    }else{
        while(true){
            int retry_times = 1;
            if(B[thread_id][next_task_id].cur==0){
                // printf("subtask - inter_dispatch - fail to dispatch in task[%d]\n",next_task_id);
                rte_pktmbuf_free(pkt);
                return;
            }
            actor_id = markid->id1.hash % B[thread_id][next_task_id].cur;
            ring = B[thread_id][next_task_id].rings[actor_id];
            // printf("subtask - inter_dispatch - task[%d]-actor[%d]-name[%s]\n",next_task_id,actor_id,ring->name);
            // if(rte_ring_count(ring) > 1020){
            //     printf("subtask - inter_dispatch - task[%d]-actor[%d]-name[%s]-count[%d]\n",next_task_id,actor_id,ring->name,rte_ring_count(ring));
            // }
            if(rte_ring_mp_enqueue(ring, pkt)!=0){
                // printf("subtask - inter_dispatch - fail to enqueue in task[%d]-actor[%d]\n",next_task_id,markid->id1.hash % B[thread_id][next_task_id].cur);
                loss[thread_id] ++;
                B[thread_id][next_task_id].loss[actor_id] +=1;
                // printf("subtask - inter_dispatch - task [%d] fail to enqueue in task[%d]-actor[%d]-loss[%d]\n",task_id,next_task_id,actor_id,B[thread_id][next_task_id].loss[actor_id]);
                if(retry_times++ > SUBTASK_DISPATCH_FAILURE_RETRY_TIMES){
                    rte_pktmbuf_free(pkt);
                    return;
                }
                next_task_id = A[thread_id][object_id][next_task_id];
                if(next_task_id == -1){
                    rte_pktmbuf_free(pkt);
                    return;
                }
            }else{
                // printf("subtask - inter_dispatch - success to dispatch in task[%d]-actor[%d]\n",next_task_id,markid->id1.hash % B[thread_id][next_task_id].cur);
                return;
            }
            
        }
    }
}

// void inter_dispatch(int cpu_id,int task_id,MARKID *markid,struct rte_mbuf *pkt){
//     struct rte_ring *ring=NULL;
//     int next_task_id = A[cpu_id][markid->id1.object_id][task_id];
//     if(next_task_id == -1){
//         rte_pktmbuf_free(pkt);
//     }else{
//         while(true){
//             if(B[cpu_id][next_task_id].range==0){
//                 rte_pktmbuf_free(pkt);
//                 return;
//             }
//             ring = B[cpu_id][next_task_id].rings[markid->id1.hash % B[cpu_id][next_task_id].range];
//             while(!ring){
//                 next_task_id = A[cpu_id][markid->id1.object_id][next_task_id];
//                 if(next_task_id == -1){
//                     rte_pktmbuf_free(pkt);
//                     return;
//                 }
//                 ring = B[cpu_id][next_task_id].rings[markid->id1.hash % B[cpu_id][next_task_id].range];
//             }
//             if(rte_ring_mp_enqueue(ring, pkt)!=0){
//                 next_task_id = A[cpu_id][markid->id1.object_id][next_task_id];
//                 if(next_task_id == -1){
//                     rte_pktmbuf_free(pkt);
//                     return;
//                 }
//             }else{
//                 return;
//             }
//         }
//     }  
// }
// void intra_dispatch(int cpu_id,int task_id,MARKID *markid,struct rte_mbuf *pkt){
//     //暂时弃用，有待优化
//     struct pipeline *p = pipeline_find(task_id,markid->id1.hash % MAX_MULTI_PIPELINE_NUM);
//     struct rte_ring *ring = p->next_pipeline_ring_in[markid->id1.hash % p->next_pipeline_num];
//     if(rte_ring_mp_enqueue(ring, pkt)!=0){
//         inter_dispatch(p->cpu_id,p->task_id, markid, pkt);
//     }
// }

unsigned int calculate_core_num(float cpu){
    int cycles = rte_rdtsc();
    return ceil(cpu /cycles);
}

