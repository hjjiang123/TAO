#ifndef THREAD_H
#define THREAD_H
#include "subtask.h"
#include "filter.h"
#ifndef THREAD_MSGQ_SIZE
#define THREAD_MSGQ_SIZE        64
#endif
#define MAX_ACTOR_NUM_PER_THREAD       8

#define NUM_MBUFS_MANAGE        64
#define MBUF_CACHE_SIZE         128
struct thread {
	uint32_t thread_id;
	bool enabled;
	bool is_exclusive; // true: exclusive, false: shared
	int actor_num;
	bool ready;
	struct actor *actors[MAX_ACTOR_NUM_PER_THREAD];
	struct rte_ring *msgq_req;
	struct rte_ring *msgq_rsp;
	FilterGroup *filter_group;
	struct pipeline_statistics stat;

	// for experiment 7.1
	int num;
};
/**
 * Main thread & data plane threads: message passing
 */
enum thread_req_type {
	THREAD_REQ_ACTOR_INSTALL = 0,
	THREAD_REQ_ACTOR_INSTALL_WITHOUT_UPDATE_B,
	THREAD_REQ_ACTOR_UNINSTALL,
	THREAD_REQ_ACTOR_TRANSFER,
	THREAD_REQ_UPDATE_A,
	THREAD_REQ_B_INIT,
	THREAD_REQ_UPDATE_B_ADD,
    THREAD_REQ_UPDATE_B_DEL,
	THREAD_REQ_B_UPDATE_DEL_TASK,
	THREAD_REQ_MAX,
	THREAD_REQ_ACTOR_MONITOR,
	THREAD_REQ_THREAD_MONITOR,
	THREAD_REQ_THREAD_EXCLUSIVE_TRANS,
	THREAD_REQ_INPUT_MONITOR,
	THREAD_REQ_INPUT_FILTERGROUP
};

struct thread_msg_req {
	enum thread_req_type type;
	union {
		struct actor *at; // for THREAD_REQ_ACTOR_INSTALL, THREAD_REQ_ACTOR_UNINSTALL, THREAD_REQ_ACTOR_INSTALL_WITHOUT_UPDATE_B
		int **losses; // for THREAD_REQ_THREAD_MONITOR
		bool is_exclusive; // for THREAD_REQ_THREAD_EXCLUSIVE_TRANS
		struct {
			struct actor *at;
			int new_thread_id;
		} actor_transfer_param; // for THREAD_REQ_ACTOR_TRANSFER
		int **A; //for THREAD_REQ_UPDATE_A
		struct {
            int task_id;
            struct rte_ring *ring;
        } delta_B; //for THREAD_REQ_UPDATE_B_ADD, THREAD_REQ_UPDATE_B_DEL
		int task_id; //for THREAD_REQ_B_UPDATE_DEL_TASK
		FilterGroup *filter_group; // for THREAD_REQ_INPUT_FILTERGROUP
	};
};

struct thread_msg_rsp {
	int status; //0:success
	union{
		struct {
			struct pipeline_statistics stat;
			bool is_retired;
			bool is_exclusive;
		} monitor_rsp; // for THREAD_REQ_ACTOR_MONITOR
		FilterGroup *retired_filter_group; // for THREAD_REQ_INPUT_FILTERGROUP
	};
};
void get_thread_attributes(int thread_id, bool *enabled, bool *is_exclusive, int *actor_num);
struct thread_msg_rsp *
thread_msg_send_recv(uint32_t thread_id,
	struct thread_msg_req *req);
void
thread_msg_free(struct thread_msg_rsp *rsp);
int
thread_init(void);

int thread_input(void *arg);

int
thread_main(void *arg);

int deploy_subtask(struct subtask *subtask);

int undeploy_subtask(struct subtask *subtask);

int deploy_actor(struct actor *actor);

int undeploy_actor(struct actor *actor);

int transfer_actor(struct actor *at, int new_thread_id);

int update_filter_group();

bool is_thread_available(int thread_id);
int get_available_thread_id();
int count_get_available_threads();
void set_thread_all_ready();

void initAB();
#endif