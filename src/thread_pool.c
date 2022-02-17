#include <stdio.h>
#include <stdlib.h>
#include "thread_pool.h"

ThreadPool* create_pool(unsigned int thread_count, void* argv, size_t element_size, void *(* func)(void *))
{
    int i;
    ThreadPool* pool = malloc(sizeof(ThreadPool));
    pool->pool = malloc(sizeof(pthread_t) * thread_count);
    pool->done_sems = malloc(sizeof(sem_t) * thread_count);
    pool->start_sems = malloc(sizeof(sem_t) * thread_count);
    pool->data_blocks = malloc(sizeof(ThreadControlBlock) * thread_count);

    pool->thread_count = thread_count;
    pool->finish_flag = 0;

    for(i = 0; i < thread_count; i++)
    {
        sem_init(pool->start_sems + i, 0, 0);
        sem_init(pool->done_sems + i, 0, 1);
    }

    for(i = 0; i < thread_count; i++)
    {
        pool->data_blocks[i].arg = argv + i * element_size;
        pool->data_blocks[i].done_sem = pool->done_sems + i;
        pool->data_blocks[i].start_sem = pool->start_sems + i;
        pool->data_blocks[i].finish_flag_pointer = &(pool->finish_flag);

        pthread_create(
            pool->pool + i,
            NULL,
            func,
            pool->data_blocks + i
        );
    }

    return pool;
}


void wait_pool(ThreadPool* pool)
{
    int i;
    for(i = 0; i < pool->thread_count; i++)
        sem_wait(pool->done_sems + i);
}


void sent_start_to_pool(ThreadPool* pool)
{
    int i;
    for(i = 0; i < pool->thread_count; i++)
        sem_post(pool->start_sems + i);
}


void destroy_pool(ThreadPool* pool)
{
    int i;
    printf("destroy waiting pool\n");
    // wait_pool(pool);                        /* -------------- */
    pool->finish_flag = 1;                  /*                */
    sent_start_to_pool(pool);               /*  等待子线程结束 */
                                            /*                */
    for(i = 0; i < pool->thread_count; i++) /*                */
        pthread_join(pool->pool[i], NULL);  /* -------------- */

    for(i = 0; i < pool->thread_count; i++)
    {
        sem_destroy(pool->start_sems + i);
        sem_destroy(pool->done_sems + i);
    }

    free(pool->start_sems);
    free(pool->done_sems);
    free(pool->pool);
    free(pool->data_blocks);
    free(pool);
}