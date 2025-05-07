#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "param.h"

// funcitons for scheduling
void sche_enqueue_subtask_params(struct subtask_params param);
int sche_InitialScheduling(void *arg);
void once_InitialScheduling(struct subtask_params *t, struct task_queue * wait_queue);
int sche_rescheduling(void *arg);
int sche_rescheduling_2(void *arg);
#endif