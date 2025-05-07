#ifndef PUBLISH_MIDDLEBOX_H
#define PUBLISH_MIDDLEBOX_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef enum {
    FREE,    // 空闲状态
    WRITING, // 写入状态
    READING  // 读取状态
} DataState;

typedef struct {
    void *data;        // 数据示例，可以根据需要扩展
    int size;        // 数据大小
    DataState state; // 当前状态
} ResultData;

typedef struct {
    ResultData *buffer; // 环形缓冲区
    int size;           // 大小
    int head;           // 当前头指针
    pthread_mutex_t lock; // 互斥锁
} ResultDataRing;


typedef int (* Init_ONE_RESULT)(void *one_res);

// 初始化环
ResultDataRing* initRing(int size,Init_ONE_RESULT init_func);

// 获取空闲指针
ResultData* getFreePointer(ResultDataRing *ring);

// 将状态改为空闲
void markAsFree(ResultDataRing *ring, ResultData *dataPtr);

// 释放环
void freeRing(ResultDataRing *ring) ;
#endif