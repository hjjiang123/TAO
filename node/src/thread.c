#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <rte_atomic.h>
#include <rte_common.h>
#include <rte_lcore.h>
#include <rte_ethdev.h>
#include <stdio.h>
#include "thread.h"

/**
 * Main thread: data plane thread context
 */
FILE *file[MAX_CORE_NUM];
static struct thread threads[MAX_CORE_NUM];

struct rte_mempool *mbuf_pool_manage;

static void
thread_free(void)
{
	uint32_t i;

	for (i = 0; i < MAX_CORE_NUM; i++) {
		struct thread *t = &threads[i];

		if (!rte_lcore_is_enabled(i))
			continue;

		/* MSGQs */
		rte_ring_free(t->msgq_req);
		rte_ring_free(t->msgq_rsp);
	}
	for (i = 0; i < MAX_CORE_NUM; i++) {
		if (file[i] != NULL) {
			fclose(file[i]);
			file[i] = NULL;
		}
	}
}

int
thread_init(void)
{
	for(int i=0;i<MAX_CORE_NUM;i++){
		char filename[NAME_MAX];
		snprintf(filename, sizeof(filename), "/home/hjjiang/hjjiang_capture_v3/node/experiment/7.1/thread%d_commad_cost", i);
		file[i] = fopen(filename, "w");
	}
	uint32_t i;

	RTE_LCORE_FOREACH_WORKER(i) {
		if(i==0 || i == 1 || i>=MAX_CORE_NUM){
			continue;
		}
		char name[NAME_MAX];
		struct rte_ring *msgq_req, *msgq_rsp;
		struct thread *t = &threads[i];
		t->thread_id = i;
		t->actor_num = 0;


		uint32_t cpu_id = rte_lcore_to_socket_id(i);

		/* MSGQs */
		snprintf(name, sizeof(name), "THREAD-%04x-MSGQ-REQ", i);

		msgq_req = rte_ring_create(name,
			THREAD_MSGQ_SIZE,
			cpu_id,
			RING_F_SP_ENQ | RING_F_SC_DEQ);

		if (msgq_req == NULL) {
			thread_free();
			return -1;

		}
		snprintf(name, sizeof(name), "THREAD-%04x-MSGQ-RSP", i);

		msgq_rsp = rte_ring_create(name,
			THREAD_MSGQ_SIZE,
			cpu_id,
			RING_F_SP_ENQ | RING_F_SC_DEQ);

		if (msgq_rsp == NULL) {
			thread_free();
			return -1;
		}
		// /* Main thread records */
		t->msgq_req = msgq_req;
		t->msgq_rsp = msgq_rsp;
		// t->ring_in = ring_in;
		t->enabled = true;
		// t->p = NULL;
		t->ready = true;
		t->filter_group = NULL;
		t->stat.read_packets = 0;
		t->stat.cycles_cost_per_duration = 0;
		t->stat.read_packets_per_duration = 0;

		t->num = 0;

	}

	// 创建管理报文内存池
	mbuf_pool_manage = rte_pktmbuf_pool_create("mempool_manage", NUM_MBUFS_DEFAULT, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);
	if (mbuf_pool_manage == NULL) {
		// 出现错误
        switch (rte_errno) {
            case E_RTE_NO_CONFIG:
                printf("thread - Error: Could not get pointer to rte_config structure\n");
                break;
            case EINVAL:
                printf("thread - Error: Invalid cache size or unaligned priv_size\n");
                break;
            case ENOSPC:
                printf("thread - Error: Maximum number of memzones already allocated\n");
                break;
            case EEXIST:
                printf("thread - Error: Memzone with the same name already exists\n");
                break;
            case ENOMEM:
                printf("thread - Error: No appropriate memory area found to create memzone\n");
                break;
            // 处理其他错误类型...
            default:
                printf("thread - Unknown error\n");
                break;
		}
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool for management\n");
	}
	initAB();
	return 0;
}
void get_thread_attributes(int thread_id, bool *enabled, bool *is_exclusive, int *actor_num){
	*enabled = threads[thread_id].enabled;
	bool flag = false;
	for(int i=0;i<threads[thread_id].actor_num;i++){
		if(threads[thread_id].actors[i]->is_exclusive){
			flag = true;
			break;
		}
	}
	threads[thread_id].is_exclusive = flag;
	*is_exclusive = flag;
	*actor_num = threads[thread_id].actor_num;
}

/**
 * Data plane threads: message handling
 */
static inline struct thread_msg_req *
thread_msg_recv(struct rte_ring *msgq_req)
{
	struct thread_msg_req *req = NULL;

	int status = rte_ring_sc_dequeue(msgq_req, (void **) &req);

	if (status != 0)
		return NULL;

	return req;
}

static inline void
thread_msg_send(struct rte_ring *msgq_rsp,
	struct thread_msg_rsp *rsp)
{
	int status;

	do {
		status = rte_ring_sp_enqueue(msgq_rsp, rsp);
	} while (status == -ENOBUFS);
}

static inline void
thread_msg_send_req(uint32_t thread_id,
	struct thread_msg_req *req)
{
	struct thread *t = &threads[thread_id];
	struct rte_ring *msgq_req = t->msgq_req;
	int status;

	do {
		status = rte_ring_sp_enqueue(msgq_req, req);
	} while (status == -ENOBUFS);

}

struct thread_msg_rsp *
thread_msg_recv_rsp(uint32_t thread_id)
{
	struct thread *t = &threads[thread_id];
	struct rte_ring *msgq_rsp = t->msgq_rsp;
	struct thread_msg_rsp *rsp;
	int status;

	do {
		status = rte_ring_sc_dequeue(msgq_rsp, (void **) &rsp);
	} while (status != 0);
	return rsp;
}

struct thread_msg_rsp *
thread_msg_send_recv(uint32_t thread_id,
	struct thread_msg_req *req)
{
	struct thread *t = &threads[thread_id];
	struct rte_ring *msgq_req = t->msgq_req;
	struct rte_ring *msgq_rsp = t->msgq_rsp;
	struct thread_msg_rsp *rsp;
	int status;
	
	/* send */
	do {
		status = rte_ring_sp_enqueue(msgq_req, req);
	} while (status == -ENOBUFS);
	/* recv */
	do {
		status = rte_ring_sc_dequeue(msgq_rsp, (void **) &rsp);
	} while (status != 0);

	return rsp;
}

static struct thread_msg_req *
thread_msg_alloc(void)
{
	size_t size = RTE_MAX(sizeof(struct thread_msg_req),
		sizeof(struct thread_msg_rsp));

	return (struct thread_msg_req *)calloc(1, size);
}

static struct thread_msg_rsp *
thread_msg_handle_actor_install(struct thread *t,
	struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *)thread_msg_alloc();

	/* Request */
	// if (t-> actor_num == MAX_ACTOR_NUM_PER_THREAD) {
	// 	rsp->status = -1;
	// 	return rsp;
	// }
	pthread_mutex_lock(&req->at->lock);
	if(t->is_exclusive)  {
		if(t->actor_num>0){
			rsp->status = -1;
			pthread_mutex_unlock(&req->at->lock);
			return rsp;
		}
	}else if(req->at->is_exclusive){
		if(t->actor_num>0){
			rsp->status = -1;
			pthread_mutex_unlock(&req->at->lock);
			return rsp;
		}else{
			t->is_exclusive = req->at->is_exclusive;
		}
	}
	pthread_mutex_unlock(&req->at->lock);
	t->actors[t->actor_num] = req->at;
	t->actor_num++;
	/* Response */
	rsp->status = 0;

	// update B
	updateB_add(t->thread_id,req->at->task_id, req->at->ring_in);

	struct thread_msg_req req1;
	req1.type = THREAD_REQ_UPDATE_B_ADD;
	req1.delta_B.task_id = req->at->task_id;
	req1.delta_B.ring = req->at->ring_in;
	int i;
	struct thread_msg_rsp *rsp1;
	RTE_LCORE_FOREACH_WORKER(i) {
		if(i == t->thread_id || i==0 ||  i==1 || i>=MAX_CORE_NUM){
			continue;
		} else {
			thread_msg_send_req(i, &req1);
			
		}
	}
	RTE_LCORE_FOREACH_WORKER(i) {
		if(i == t->thread_id || i==0 ||  i==1 || i>=MAX_CORE_NUM){
			continue;
		} else {
			rsp1 = thread_msg_recv_rsp(i);
			thread_msg_free(rsp1);
		}
	}
	return rsp;
}
static struct thread_msg_rsp *
thread_msg_handle_actor_install_without_update_B(struct thread *t,
	struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *)thread_msg_alloc();

	/* Request */
	// if (t-> actor_num == MAX_ACTOR_NUM_PER_THREAD) {
	// 	rsp->status = -1;
	// 	return rsp;
	// }
	pthread_mutex_lock(&req->at->lock);
	if(t->is_exclusive)  {
		if(t->actor_num>0){
			rsp->status = -1;
			pthread_mutex_unlock(&req->at->lock);
			#ifdef DEBUG
			printf("thread - Error: exclusive conflict 1 in thread_msg_handle_actor_install_without_update_B.\n");
			#endif
			return rsp;
		}
	}else if(req->at->is_exclusive){
		if(t->actor_num>0){
			rsp->status = -1;
			pthread_mutex_unlock(&req->at->lock);
			#ifdef DEBUG
			printf("thread - Error: exclusive conflict 2 in thread_msg_handle_actor_install_without_update_B.\n");
			#endif
			return rsp;
		}else{
			t->is_exclusive = req->at->is_exclusive;
		}
	}
	pthread_mutex_unlock(&req->at->lock);
	t->actors[t->actor_num] = req->at;
	t->actor_num++;
	/* Response */
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *
thread_msg_handle_actor_uninstall(struct thread *t,
	struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *)thread_msg_alloc();

	/* Request */
	if (t-> actor_num == 0) { 
		rsp->status = -1;
		return rsp;
	}
	int i;
	for(i=0;i<t->actor_num;i++){
		if(t->actors[i] == req->at){
			break;
		}
	}
	if(i == t->actor_num){
		rsp->status = -1;
		return rsp;
	}
	for(int j=i;j<t->actor_num-1;j++){
		t->actors[j] = t->actors[j+1];
	}
	t->actor_num--;
	if(t->actor_num == 0){
		t->is_exclusive = false;
	}


	/* Response */
	rsp->status = 0;
	// update B
	struct thread_msg_req req1;
	req1.type = THREAD_REQ_UPDATE_B_DEL;
	req1.delta_B.task_id = req->at->task_id;
	req1.delta_B.ring = req->at->ring_in;
	updateB_del(t->thread_id,req->at->task_id, req->at->ring_in);
	struct thread_msg_rsp *rsp1;
	RTE_LCORE_FOREACH_WORKER(i) {
		if(i == t->thread_id || i==0|| i==1 || i>=MAX_CORE_NUM){
			continue;
		} else {
			thread_msg_send_req(i, &req1);
		}
	}
	RTE_LCORE_FOREACH_WORKER(i) {
		if(i == t->thread_id || i==0|| i==1 || i>=MAX_CORE_NUM){
			continue;
		} else {
			rsp1 = thread_msg_recv_rsp(i);
			thread_msg_free(rsp1);
		}
	}
	return rsp;
}
static struct thread_msg_rsp *
thread_msg_handle_actor_transfer(struct thread *t,
	struct thread_msg_req *req){
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *)thread_msg_alloc();
	struct actor *actor = req->actor_transfer_param.at;
	/* Request */
	if (t-> actor_num == 0) { 
		rsp->status = -1;
		return rsp;
	}
	int i;
	for(i=0;i<t->actor_num;i++){
		if(t->actors[i] == actor){
			break;
		}
	}
	if(i == t->actor_num){
		rsp->status = -1;
		#ifdef DEBUG
		printf("thread - Error: actor not found\n");
		#endif
		return rsp;
	}
	for(int j=i;j<t->actor_num-1;j++){
		t->actors[j] = t->actors[j+1];
	}
	t->actor_num--;
	if(t->actor_num == 0){
		t->is_exclusive = false;
	}
	actor->thread_id = req->actor_transfer_param.new_thread_id;

	int status;

	/* Write request */
	req->type = THREAD_REQ_ACTOR_INSTALL_WITHOUT_UPDATE_B;
	req->at = actor;

	/* Send request and wait for response */
	struct thread_msg_rsp *rsp1 = thread_msg_send_recv(actor->thread_id, req);
	thread_msg_free(rsp1);
	/* Response */
	rsp->status = 0;

	/* Read response */
	return rsp;
}
static struct thread_msg_rsp *
thread_msg_handle_update_A(struct thread *t,
	struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	int thread_id = t->thread_id;
	update_A(thread_id, req->A);
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *
thread_msg_handle_update_B(struct thread *t,
	struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	int thread_id = t->thread_id;
	if(req->type==THREAD_REQ_UPDATE_B_ADD){
		updateB_add(thread_id,req->delta_B.task_id, req->delta_B.ring);
	}else if (req->type==THREAD_REQ_UPDATE_B_DEL){
		updateB_del(thread_id,req->delta_B.task_id, req->delta_B.ring);
	}else if (req->type==THREAD_REQ_B_UPDATE_DEL_TASK){
		updateB_del_task(thread_id,req->task_id);
	}
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *thread_msg_handle_actor_monitor(struct thread *t, struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	struct actor *actor = req->at;
	actor->p->stat.read_packets += actor->p->stat.read_packets_per_duration;
	rsp->monitor_rsp.stat = actor->p->stat;
	rsp->monitor_rsp.stat.loss = find_thread_loss(t->thread_id);

	pthread_mutex_lock(&actor->lock);
	rsp->monitor_rsp.is_retired = actor->is_retired;
	rsp->monitor_rsp.is_exclusive = actor->is_exclusive;
	pthread_mutex_unlock(&actor->lock);
	actor->p->stat.read_packets_per_duration = 0;
	actor->p->stat.cycles_cost_per_duration = 0;
	actor->p->stat.num_0 = 0;
	actor->p->stat.num_1 = 0;
	actor->p->stat.num_2 = 0;
	actor->p->stat.loss = 0;
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *thread_msg_handle_thread_monitor(struct thread *t, struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	copy_B_loss(t->thread_id,req->losses);
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *thread_msg_handle_input_monitor(struct thread *t, struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	t->stat.read_packets += t->stat.read_packets_per_duration;
	rsp->monitor_rsp.stat = t->stat;
	rsp->monitor_rsp.stat.loss = find_thread_loss(t->thread_id);
	if(t->filter_group){
		t->filter_group->total_match_count += t->filter_group->match_count;
		t->filter_group->total_pkt_count += t->filter_group->pkt_count;
		
		rsp->monitor_rsp.stat.filter_num = t->filter_group->count;
		rsp->monitor_rsp.stat.match_count = t->filter_group->match_count;
		rsp->monitor_rsp.stat.pkt_count = t->filter_group->pkt_count;
		rsp->monitor_rsp.stat.total_match_count = t->filter_group->total_match_count;
		rsp->monitor_rsp.stat.total_pkt_count = t->filter_group->total_pkt_count;
		t->filter_group->match_count = 0;
		t->filter_group->pkt_count = 0;
	}else{
		rsp->monitor_rsp.stat.filter_num = 0;
		rsp->monitor_rsp.stat.match_count = 0;
		rsp->monitor_rsp.stat.pkt_count = 0;
		rsp->monitor_rsp.stat.total_match_count = 0;
		rsp->monitor_rsp.stat.total_pkt_count = 0;
	}
	
	t->stat.read_packets_per_duration = 0;
	t->stat.cycles_cost_per_duration = 0;
	t->stat.loss += rsp->monitor_rsp.stat.loss;
	t->stat.num_0 = 0;
	t->stat.num_1 = 0;
	t->stat.num_2 = 0;
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *thread_msg_handle_thread_exclusive_trans(struct thread *t, struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	t->is_exclusive = req->is_exclusive;
	rsp->status = 0;
	return rsp;
}

static struct thread_msg_rsp *thread_msg_handle_input_filtergroup(struct thread *t, struct thread_msg_req *req)
{
	struct thread_msg_rsp *rsp = (struct thread_msg_rsp *) malloc(sizeof(struct thread_msg_rsp));
	rsp->status = 0;
	rsp->retired_filter_group = t->filter_group;
	t->filter_group = req->filter_group;
	return rsp;
}

static void
thread_msg_handle(struct thread *t)
{
	// int ring_lens[t->actor_num];
	// int new_ring_lens[t->actor_num];
	// for(int i=0;i<t->actor_num;i++){
	// 	ring_lens[i] = rte_ring_count(t->actors[i]->ring_in);
	// }
	
	for ( ; ; ) {
		#ifdef DEBUG_7_1
		uint64_t start_tsc_eachtask,end_tsc_eachtask;
		start_tsc_eachtask = rte_rdtsc();
		#endif
		struct thread_msg_req *req;
		struct thread_msg_rsp *rsp;

		req = thread_msg_recv(t->msgq_req);
		if (req == NULL)
			break;
		int type = req->type;
		switch (type) {
			case THREAD_REQ_ACTOR_INSTALL:
				rsp = thread_msg_handle_actor_install(t, req);
				break;
			case THREAD_REQ_ACTOR_INSTALL_WITHOUT_UPDATE_B:
				rsp = thread_msg_handle_actor_install_without_update_B(t, req);
				break;
			case THREAD_REQ_ACTOR_UNINSTALL:
				rsp = thread_msg_handle_actor_uninstall(t, req);
				break;
			case THREAD_REQ_ACTOR_TRANSFER:
				rsp = thread_msg_handle_actor_transfer(t, req);
				break;
			case THREAD_REQ_UPDATE_A: 
				rsp = thread_msg_handle_update_A(t, req);
				break;
			case THREAD_REQ_B_INIT:
				rsp = thread_msg_handle_update_B(t, req);
				break;
			case THREAD_REQ_UPDATE_B_ADD:
				rsp = thread_msg_handle_update_B(t, req);
				break;
			case THREAD_REQ_UPDATE_B_DEL:
				rsp = thread_msg_handle_update_B(t, req);
				break;
			case THREAD_REQ_B_UPDATE_DEL_TASK:
				rsp = thread_msg_handle_update_B(t, req);
				break;
			case THREAD_REQ_THREAD_MONITOR:
				rsp = thread_msg_handle_thread_monitor(t, req);
				break;
			case THREAD_REQ_THREAD_EXCLUSIVE_TRANS:
				rsp = thread_msg_handle_thread_exclusive_trans(t, req);
				break;
			case THREAD_REQ_ACTOR_MONITOR:
				rsp = thread_msg_handle_actor_monitor(t, req);
				break;
			case THREAD_REQ_INPUT_MONITOR:
				rsp = thread_msg_handle_input_monitor(t, req);
				break;
			case THREAD_REQ_INPUT_FILTERGROUP:
				rsp = thread_msg_handle_input_filtergroup(t, req);
				break;
			default:
				rsp = (struct thread_msg_rsp *) req;
				rsp->status = -1;
		}

		thread_msg_send(t->msgq_rsp, rsp);
		// int max_delta_ring_len = 0;
		// for(int i=0;i<t->actor_num;i++){
		// 	new_ring_lens[i] = rte_ring_count(t->actors[i]->ring_in);
		// 	if(new_ring_lens[i] - ring_lens[i] > max_delta_ring_len){
		// 		max_delta_ring_len = new_ring_lens[i] - ring_lens[i];
		// 	}
		// }
		
		#ifdef DEBUG_7_1
		end_tsc_eachtask = rte_rdtsc();
		fprintf(file[t->thread_id],"thread - thread[%d] command[%d]: cycles = %6d\n",t->thread_id,type,end_tsc_eachtask-start_tsc_eachtask);
		fflush(file[t->thread_id]);
		// if(t->num<100 && t->actor_num!=0 && type == THREAD_REQ_ACTOR_TRANSFER){
		// 	printf("thread - thread %d with command %d, times: %d: ",t->thread_id,type,t->num);
		// 	// for(int i=0;i<t->actor_num;i++){
		// 	// 	printf("ring_lens[%d]=%d ", i, new_ring_lens[i]);
		// 	// }
		// 	printf("\n");
		// 	// printf("thread - thread %d: max_delta_ring_len = %d\n",t->thread_id,max_delta_ring_len);
		// 	fprintf(file[t->thread_id],"thread - thread %d: max_delta_ring_len = %d\n",t->thread_id,max_delta_ring_len);
		// 	fflush(file[t->thread_id]);
		// 	t->num++;
		// }else if(t->num==100){
		// 	printf("thread - thread %d: max_delta_ring_len count end!\n",t->thread_id);
		// 	fclose(file[t->thread_id]);
		// 	t->num++;
		// }
		#endif
	}
	
}



bool check_subtask_deployable(struct subtask *subtask){
    if (subtask->actor_num == 0)
        return false;
    for (int i = 0; i < subtask->cpu; i++){
        if (subtask->actors[i]->p->plugin_num == 0
            ||!rte_lcore_is_enabled(subtask->actors[i]->thread_id))
            return false;
    }
    return true;
}
/**
 * Main thread
 */

void
thread_msg_free(struct thread_msg_rsp *rsp)
{
	free(rsp);
}

int deploy_subtask(struct subtask *subtask){
    if(!check_subtask_deployable(subtask))
        return -1;
    struct thread_msg_req *req = thread_msg_alloc();
	int status;
	if (req == NULL)
		return -1;
    for (int i = 0; i < subtask->cpu; i++){
        /* Write request */
		printf("thread - install task[%d]'s actor [%d]\n",subtask->task_id, subtask->actors[i]->actor_id);
        req->type = THREAD_REQ_ACTOR_INSTALL;
        req->at = subtask->actors[i];

        /* Send request and wait for response */
        struct thread_msg_rsp *rsp = thread_msg_send_recv(subtask->actors[i]->thread_id, req);
		thread_msg_free(rsp);
    }

	return 0;
}
int undeploy_subtask(struct subtask *subtask){
	struct thread_msg_req *req = thread_msg_alloc();
	struct thread_msg_rsp *rsp;
	int status;
	if (req == NULL)
		return -1;
	for (int i = 0; i < subtask->actor_num; i++){
		/* Write request */
		status = undeploy_actor(subtask->actors[i]);
		if(status) //failed
			printf("thread - actor %d failed to undeploy\n",subtask->actors[i]->actor_id);


			return status;
	}


	return 0;
}

int deploy_actor(struct actor *actor){
	struct thread_msg_req *req = thread_msg_alloc();
	int status;
	if (req == NULL)
		return -1;
	/* Write request */
	req->type = THREAD_REQ_ACTOR_INSTALL;
	req->at = actor;
	
	/* Send request and wait for response */
	struct thread_msg_rsp *rsp = thread_msg_send_recv(actor->thread_id, req);
	thread_msg_free(rsp);


	return 0;
}

int undeploy_actor(struct actor *actor){
	struct thread_msg_req *req = thread_msg_alloc();
	int status;
	if (req == NULL)
		return -1;
	
	/* Write request */
	req->type = THREAD_REQ_ACTOR_UNINSTALL;
	req->at = actor;

	/* Send request and wait for response */
	struct thread_msg_rsp *rsp = thread_msg_send_recv(actor->thread_id, req);
	thread_msg_free(rsp);
	
	/* Request completion */
	return 0;
}

int transfer_actor(struct actor *at, int new_thread_id){
	struct thread_msg_req *req = thread_msg_alloc();
	if (req == NULL)
		return -1;
	/* Write request */
	req->type = THREAD_REQ_ACTOR_TRANSFER;
	req->actor_transfer_param.at = at;
	req->actor_transfer_param.new_thread_id = new_thread_id;

	/* Send request and wait for response */
	struct thread_msg_rsp *rsp = thread_msg_send_recv(at->thread_id, req);
	thread_msg_free(rsp);

	/* Request completion */
	return 0;
}

int update_filter_group(){
	struct thread_msg_req *req = thread_msg_alloc();
	struct thread_msg_rsp *rsp;
	int status;
	if (req == NULL)
		return -1;
	/* Send request and wait for response */
	for(int i=2;i<2+THREAD_INPUT_THREAD_NUM;i++){
		FilterGroup *filter_group = create_filter_group();
		req->type = THREAD_REQ_INPUT_FILTERGROUP;
		req->filter_group = filter_group;
		rsp = thread_msg_send_recv(i, req);
		/* Read response */
		status = rsp->status;
		FilterGroup *retired_filter_group = rsp->retired_filter_group;
		free_filter_group(retired_filter_group);
		/* Free response */
		thread_msg_free(rsp);
	}

	/* Request completion */
	return status;
}

static inline int
thread_is_running(uint32_t thread_id)
{
	enum rte_lcore_state_t thread_state;

	thread_state = rte_eal_get_lcore_state(thread_id);
	return (thread_state == RUNNING) ? 1 : 0;
}

static inline int
actor_is_running(struct actor *actor)
{
	return thread_is_running(actor->thread_id);
}


bool is_thread_available(int thread_id){
	if(thread_id>=2+THREAD_INPUT_THREAD_NUM && threads[thread_id].enabled && thread_is_running(thread_id))
		return true;
	return false;
}

int get_available_thread_id(){  //for exculsive
	int k = 0;
	for(int i=2+THREAD_INPUT_THREAD_NUM;i<MAX_CORE_NUM;i++){
		if((threads[i].enabled) && thread_is_running(i) && (threads[i].actor_num==0) && threads[i].ready==true){
			threads[i].ready=false;
			return i;
		}
	}
    return -1;
}

int count_get_available_threads(){ 
	int count = 0;
	for(int i=2+THREAD_INPUT_THREAD_NUM;i<MAX_CORE_NUM;i++){
		if(threads[i].enabled && thread_is_running(i) && threads[i].actor_num==0 && threads[i].ready==true){
			count ++;
		}
	}
    return count;
}

void set_thread_all_ready(){
	for(int i=2+THREAD_INPUT_THREAD_NUM;i<MAX_CORE_NUM;i++){
		threads[i].ready = true;
	}
}

/**
* Data plane threads: main
*/
int
thread_main(void *arg)
{
	uint32_t thread_id = rte_lcore_id();
	struct rte_mbuf *pkts[BURST_SIZE];
	MARKID *markid=(MARKID *)malloc(sizeof(MARKID));
	struct thread *t = &threads[thread_id];
	uint64_t start_tsc, end_tsc, diff_tsc;
	uint64_t start_tsc_eachtask, end_tsc_eachtask;
	uint64_t start_tsc_eachtask1, end_tsc_eachtask1;
	uint64_t freq = rte_get_timer_hz();
	bool flag = false;
	struct actor *actor;
	struct rte_ring *ring_in;
	/* Dispatch loop */
	while (1){
		for(int k=0;k<128;k++){
			for(int i=0;i<t->actor_num;i++){
				start_tsc_eachtask = rte_rdtsc();
				actor = t->actors[i];
				ring_in = actor->ring_in;
				int dequeue_count = rte_ring_dequeue_burst(ring_in, (void **)pkts, BURST_SIZE, NULL);
				if (unlikely(dequeue_count == 0)){
					actor->p->stat.num_0 ++;
				}else{
					if(dequeue_count != BURST_SIZE){
						actor->p->stat.num_1 ++;
					}else{
						actor->p->stat.num_2 ++;
					}
					if(actor->is_retired){
						for(int i=0;i<dequeue_count;i++){
							actor->dispatch(actor->thread_id,actor->task_id, markid, pkts[i]);
						}
					}else{
						for(int i=0;i<dequeue_count;i++){
							markid->id2 = pkts[i]->hash.fdir.lo;
							run_pipeline(actor->p, pkts[i]);
							// start_tsc_eachtask1 = rte_rdtsc();
							actor->dispatch(actor->thread_id,actor->task_id, markid, pkts[i]);
							// end_tsc_eachtask1 = rte_rdtsc();
						}
					}
				}
				
				actor->run_actor_times++;
				if(actor->run_actor_times == actor->run_actor_times_per_manage){
					actor->run_actor_times = 0;
					thread_msg_handle(t);
				}
				end_tsc_eachtask = rte_rdtsc();
				// actor retire
				if(end_tsc_eachtask > actor->retired_time){
					printf("thread - task %d actor %d retired in %" PRIu64 " > %" PRIu64 "\n",actor->task_id, actor->actor_id, end_tsc_eachtask, actor->retired_time);
					actor->is_retired = true;
				}
				if(dequeue_count){
					actor->p->stat.cycles_cost_per_duration += end_tsc_eachtask - start_tsc_eachtask;
					actor->p->stat.read_packets_per_duration += dequeue_count;
				}
			}
		}
		thread_msg_handle(t);
		
	}
	return 0;
}

int thread_input(void *arg){
    unsigned int thread_id = rte_lcore_id();
	int queue_id = *(int *)arg;
	struct thread *t = &threads[thread_id];
    struct rte_mbuf *mbufs[BURST_SIZE];
    MARKID *markid=(MARKID *)malloc(sizeof(MARKID));
	uint64_t start_tsc, end_tsc;
	uint64_t freq = rte_get_timer_hz();
	bool flag = false;
	int status;   
	struct thread_msg_req *req;
	while (1)
    {
        // 从环形缓冲区中取出一批报文，循环此过程128次
        for(int k=0;k<128;k++){
			const uint16_t nb_rx = rte_eth_rx_burst(DPDK_PORT, queue_id, mbufs, BURST_SIZE);
			if (unlikely(nb_rx == 0))
				continue;
			start_tsc = rte_rdtsc();
			
			for (uint16_t i = 0; i < nb_rx; ++i)
			{
				mbufs[i]->hash.fdir.id = matchFilter(t->filter_group,mbufs[i]);
				markid->id2 = mbufs[i]->hash.fdir.lo;
				inter_dispatch(thread_id,MAX_TASK_NUM,markid, mbufs[i]);
			}
			end_tsc = rte_rdtsc();
			t->stat.cycles_cost_per_duration += end_tsc - start_tsc;
			t->stat.read_packets_per_duration += nb_rx;
		}
		
		// 管理消息
		thread_msg_handle(t);
        rte_pause();
    }
}

