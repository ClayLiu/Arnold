#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "arnold_kernel.h"
#include "pooling_arnold.h"

const unsigned int sub_thread_count = 32;
static char finish_flag = 0;
static pthread_mutex_t sub_lock;

// 线程池子线程的数据控制块
typedef struct thread_control_block
{
    unsigned int kernel_size;
    unsigned int take_size;
    char* buffer_and_offset;   
    sem_t* start_sem_p;
    sem_t* done_sem_p;
}ThreadControlBlock;


void* kernel_thread(void* arg)
{
    unsigned int take_size = ((ThreadControlBlock*)arg)->take_size;
    unsigned int kernel_size = ((ThreadControlBlock*)arg)->kernel_size;
    char* buffer_and_offset = ((ThreadControlBlock*)arg)->buffer_and_offset;   
    sem_t* start_sem_p = ((ThreadControlBlock*)arg)->start_sem_p;
    sem_t* done_sem_p = ((ThreadControlBlock*)arg)->done_sem_p;

    sem_post(done_sem_p);

    int i;
    unsigned int block_size = kernel_size * kernel_size;
    unsigned int block_count = take_size / block_size;
    char* take_buffer = malloc(sizeof(char) * take_size);
    char* temp = malloc(sizeof(char) * block_size);
    ArnoldMap* map = build_map(kernel_size);

    // printf("take_buffer = %p, temp = %p\n", take_buffer, temp);
    if(take_buffer == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in kernel_thread on take_buffer.");
        exit(1);
    }

    if(temp == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in kernel_thread on temp.");
        exit(1);
    }

    while(1)
    {
        sem_wait(start_sem_p);
        if(finish_flag)
            break;

        // 子线程摘取要处理的部分
        pthread_mutex_lock(&sub_lock);
        memcpy(take_buffer, buffer_and_offset, take_size);
        pthread_mutex_unlock(&sub_lock);

        // for(i = 0; i < block_count; i++)
        //     arnold_ize(
        //         take_buffer + i * block_size,
        //         temp,
        //         kernel_size
        //     );


        for(i = 0; i < block_count; i++)
            arnold_ize_using_map(
                take_buffer + i * block_size,
                temp,
                map
            );

        // 把处理完成的部分挂回去
        pthread_mutex_lock(&sub_lock);
        memcpy(buffer_and_offset, take_buffer, take_size);
        pthread_mutex_unlock(&sub_lock);
        
        sem_post(done_sem_p);
    }

    free(temp);
    free(take_buffer);
    destroy_map(map);
    // printf("done sub\n");
}


ThreadPool* create_pool(unsigned int single_buffer_size, unsigned int kernel_size, char* buffer)
{
    int i;
    unsigned int single_sub_take = single_buffer_size / sub_thread_count;

    ThreadPool* pool = malloc(sizeof(ThreadPool));
    pool->pool = malloc(sizeof(pthread_t) * sub_thread_count);
    pool->start_sems = malloc(sizeof(sem_t) * sub_thread_count);
    pool->done_sems = malloc(sizeof(sem_t) * sub_thread_count);

    ThreadControlBlock blocks[sub_thread_count];

    for(i = 0; i < sub_thread_count; i++)
    {
        sem_init(pool->start_sems + i, 0, 0);
        sem_init(pool->done_sems + i, 0, 0);
    }

    for(i = 0; i < sub_thread_count; i++)
    {
        blocks[i].kernel_size = kernel_size;
        blocks[i].take_size = single_sub_take;
        blocks[i].start_sem_p = pool->start_sems + i;
        blocks[i].done_sem_p = pool->done_sems + i;
        blocks[i].buffer_and_offset = buffer + i * single_sub_take;
    }

    for(i = 0; i < sub_thread_count; i++)
        pthread_create(
            pool->pool + i,
            NULL,
            kernel_thread,
            blocks + i
        );
    
    wait_pool(pool);
    return pool;
}


void destroy_pool(ThreadPool* pool)
{
    int i;
    // printf("destroy waiting pool\n");
    // wait_pool(pool);
    finish_flag = 1; 
    sent_start_to_pool(pool);               
                                                               
    for(i = 0; i < sub_thread_count; i++) 
        pthread_join(pool->pool[i], NULL);

    for(i = 0; i < sub_thread_count; i++)
    {
        sem_destroy(pool->start_sems + i);
        sem_destroy(pool->done_sems + i);
    }

    free(pool->start_sems);
    free(pool->done_sems);
    free(pool->pool);
    free(pool);
}


void wait_pool(ThreadPool* pool)
{
    int i;
    for(i = 0; i < sub_thread_count; i++)
        sem_wait(pool->done_sems + i);
}


void sent_start_to_pool(ThreadPool* pool)
{
    int i;
    for(i = 0; i < sub_thread_count; i++)
        sem_post(pool->start_sems + i);
}