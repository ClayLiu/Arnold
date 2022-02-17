#ifndef POOLING_ARNOLD_H_HDIFHD
#define POOLING_ARNOLD_H_HDIFHD

#include <pthread.h>
#include <semaphore.h>

typedef struct thread_pool
{
    pthread_t* pool;
    sem_t* start_sems;
    sem_t* done_sems;
}ThreadPool;


ThreadPool* create_pool(unsigned int single_buffer_size, unsigned int kernel_size, char* buffer);

void destroy_pool(ThreadPool* pool);

void wait_pool(ThreadPool* pool);

void sent_start_to_pool(ThreadPool* pool);

#endif