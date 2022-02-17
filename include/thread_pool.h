#ifndef THREAD_POOL_HDIFHD
#define THREAD_POOL_HDIFHD

#include <pthread.h>
#include <semaphore.h>


// 线程池子线程的数据控制块
typedef struct thread_control_block
{
    void* arg;
    
    sem_t* start_sem;
    sem_t* done_sem;
    char* finish_flag_pointer;
}ThreadControlBlock;


typedef struct thread_pool
{
    pthread_t* pool;
    sem_t* start_sems;
    sem_t* done_sems;
    ThreadControlBlock* data_blocks;
    unsigned int thread_count;
    char finish_flag;
}ThreadPool;


// 创建无返回值的线程池
ThreadPool* create_pool(unsigned int thread_count, void* argv, size_t element_size, void *(* func)(void *));

// 等待线程池完成任务
void wait_pool(ThreadPool* pool);

// 向线程池发送执行任务
void sent_start_to_pool(ThreadPool* pool);

// 销毁线程池
void destroy_pool(ThreadPool* pool);

#endif