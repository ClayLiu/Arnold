#include <stdio.h>
#include <stdlib.h>
#include "unique_mutex_queue.h"

static int hash(UniqueMutexQueue* um_queue, elem_type value)
{
    int original_hash_index = (unsigned long long) value % um_queue->max_queue_length;
    int hash_index          = original_hash_index;

    while(
        um_queue->hash_table[hash_index] != NULL &&
        um_queue->hash_table[hash_index] != value
    )
    {
        hash_index++;
        hash_index %= um_queue->max_queue_length;
        
        if(hash_index == original_hash_index)
            return -1;
    }

    return hash_index;
}


UniqueMutexQueue* new_unique_mutex_queue(unsigned int max_queue_length)
{
    int i;
    UniqueMutexQueue* um_queue = malloc(sizeof(UniqueMutexQueue));
    
    um_queue->max_queue_length = max_queue_length;
    um_queue->read_index = 0;
    um_queue->write_index = 0;
    
    um_queue->queue         = malloc(sizeof(elem_type) * max_queue_length);
    um_queue->size_queue    = malloc(sizeof(size_t) * max_queue_length);
    um_queue->hash_table    = malloc(sizeof(elem_type) * max_queue_length);
    um_queue->hash_sems     = malloc(sizeof(sem_t) * max_queue_length);

    pthread_mutex_init(&(um_queue->lock), NULL);
    sem_init(&(um_queue->full), 0, 0);
    sem_init(&(um_queue->empty), 0, max_queue_length);

    for(i = 0; i < max_queue_length; i++)
    {
        um_queue->hash_table[i] = NULL;
        sem_init(um_queue->hash_sems + i, 0, 1);
    }

    return um_queue;
}


void destroy_unique_mutex_queue(UniqueMutexQueue* um_queue)
{
    int i;

    free(um_queue->queue);
    um_queue->queue = NULL;

    free(um_queue->size_queue);
    um_queue->size_queue = NULL;

    free(um_queue->hash_table);
    um_queue->hash_table = NULL;

    for(i = 0; i < um_queue->max_queue_length; i++)
        sem_destroy(um_queue->hash_sems + i);

    free(um_queue->hash_sems);

    sem_destroy(&(um_queue->full));
    sem_destroy(&(um_queue->empty));
    free(um_queue);    
}


void get_queue_head(
    UniqueMutexQueue* um_queue, 
    elem_type* ret_val_address,
    size_t* ret_size_address
)
{
    elem_type return_value;

    sem_wait(&(um_queue->full));
    pthread_mutex_lock(&(um_queue->lock));

    *ret_val_address = um_queue->queue[um_queue->read_index];
    *ret_size_address = um_queue->size_queue[um_queue->read_index];

    sem_post(&(um_queue->full));
    pthread_mutex_unlock(&(um_queue->lock));
}


void release_queue_head(UniqueMutexQueue* um_queue)
{
    int hash_index;

    sem_wait(&(um_queue->full));
    pthread_mutex_lock(&(um_queue->lock));

    hash_index = hash(
        um_queue, 
        um_queue->queue[um_queue->read_index]
    );
    sem_post(um_queue->hash_sems + hash_index);

    um_queue->read_index++;
    um_queue->read_index %= um_queue->max_queue_length;
    
    sem_post(&(um_queue->empty));
    pthread_mutex_unlock(&(um_queue->lock));
}


void join_queue(UniqueMutexQueue* um_queue, elem_type value)
{
    int hash_index;

    sem_wait(&(um_queue->empty));
    pthread_mutex_lock(&(um_queue->lock));
    
    hash_index = hash(um_queue, value);
    
    sem_wait(um_queue->hash_sems + hash_index);

    um_queue->hash_table[hash_index]        = 
    um_queue->queue[um_queue->write_index]  = value;

    um_queue->write_index++;
    um_queue->write_index %= um_queue->max_queue_length;
    
    sem_post(&(um_queue->empty));
    pthread_mutex_unlock(&(um_queue->lock));
}


void join_size(UniqueMutexQueue* um_queue, size_t size)
{
    pthread_mutex_lock(&(um_queue->lock));
    
    um_queue->size_queue[
        (um_queue->write_index - 1) % um_queue->max_queue_length
    ] = size;
    
    sem_post(&(um_queue->full));
    pthread_mutex_unlock(&(um_queue->lock));
}