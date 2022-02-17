#ifndef HASH_unique_mutex_queue_H_HDIFHD
#define HASH_unique_mutex_queue_H_HDIFHD

#include <pthread.h>
#include <semaphore.h>

typedef char* elem_type;  // 此散列互斥队列，元素类型定为 char*

typedef struct unique_mutex_queue_t
{
    unsigned int max_queue_length;
    unsigned int read_index; 
    unsigned int write_index;

    elem_type* queue;
    size_t* size_queue;
    elem_type* hash_table;
    sem_t* hash_sems;

    pthread_mutex_t lock;
    sem_t empty, full;
}UniqueMutexQueue;

UniqueMutexQueue* new_unique_mutex_queue(unsigned int max_queue_length);

void destroy_unique_mutex_queue(UniqueMutexQueue* um_queue);

void get_queue_head(
    UniqueMutexQueue* um_queue, 
    elem_type* ret_val_address,
    size_t* ret_size_address
);

void release_queue_head(UniqueMutexQueue* um_queue);

void join_queue(UniqueMutexQueue* um_queue, elem_type value);

void join_size(UniqueMutexQueue* um_queue, size_t size);

#endif