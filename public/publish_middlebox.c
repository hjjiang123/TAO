#include "publish_middlebox.h"

// 初始化环
ResultDataRing* initRing(int size,Init_ONE_RESULT init_func){
    ResultDataRing *ring = (ResultDataRing*)malloc(sizeof(ResultDataRing));
    ring->size = size;
    ring->head = 0;
    ring->buffer = (ResultData*)malloc(sizeof(ResultData) * size);
    for(int i=0;i<size;i++){
        void *one_res;
        int size = init_func(one_res);
        ring->buffer[i].data = one_res;
        ring->buffer[i].state = FREE; // 初始化为FREE状态
    }
    pthread_mutex_init(&ring->lock, NULL); // 初始化互斥锁
    return ring;
}

// 获取空闲指针
ResultData* getFreePointer(ResultDataRing *ring) {
    pthread_mutex_lock(&ring->lock); // 加锁
    for (int i = 0; i < ring->size; i++) {
        int index = (ring->head + i) % ring->size;
        if (ring->buffer[index].state == FREE) {
            ring->buffer[index].state = WRITING; // 设置为WRITING状态
            pthread_mutex_unlock(&ring->lock); // 解锁
            return &ring->buffer[index];
        }
    }
    pthread_mutex_unlock(&ring->lock); // 解锁
    return NULL; // 没有空闲指针
}

// 将状态改为空闲   
void markAsFree(ResultDataRing *ring, ResultData *dataPtr) {
    pthread_mutex_lock(&ring->lock); // 加锁
    if (dataPtr->state == READING) {
        dataPtr->state = FREE; // 设置为FREE状态
    } else {
        printf("数据状态错误，无法标记为空闲!\n");
    }
    pthread_mutex_unlock(&ring->lock); // 解锁
}

// 释放环
void freeRing(ResultDataRing *ring) {
    pthread_mutex_destroy(&ring->lock); // 销毁互斥锁
    for(int i=0;i<ring->size;i++){
        free(ring->buffer[i].data);        
    }
    free(ring->buffer);
    free(ring);
}