#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mutex_buffer_pool.h"

MutexBufferPool* new_mutex_buffer_pool(unsigned int buffer_count, size_t buffer_size)
{
    MutexBufferPool* buffer_pool = malloc(sizeof(MutexBufferPool));
    buffer_pool->buffer_count = buffer_count;
    buffer_pool->buffer_size = buffer_size;
    buffer_pool->read_index = 0;
    buffer_pool->write_index = 0;
    buffer_pool->real_sizes = malloc(sizeof(size_t) * buffer_count);
    buffer_pool->buffer_pointer = malloc(sizeof(char) * buffer_size * buffer_count);
    
    if(buffer_pool->buffer_pointer == NULL)
    {
        fprintf(stderr, "(Error) Memory failed in new_mutex_buffer\n");
        exit(1);
    }

    pthread_mutex_init(&(buffer_pool->lock), NULL);
    sem_init(&(buffer_pool->full), 0, 0);
    sem_init(&(buffer_pool->empty), 0, buffer_count);

    return buffer_pool;
}


void destroy_mutex_buffer_pool(MutexBufferPool* buffer_pool)
{
    free(buffer_pool->real_sizes);
    buffer_pool->real_sizes = NULL;

    free(buffer_pool->buffer_pointer);
    buffer_pool->buffer_pointer = NULL;

    sem_destroy(&(buffer_pool->full));
    sem_destroy(&(buffer_pool->empty));
    free(buffer_pool);
}


size_t read_buffer(MutexBufferPool* buffer_pool, char* dst)
{
    size_t read_size;

    sem_wait(&(buffer_pool->full));
    pthread_mutex_lock(&(buffer_pool->lock));
    
    memcpy(
        dst, 
        buffer_pool->buffer_pointer + buffer_pool->read_index * buffer_pool->buffer_size, 
        read_size = buffer_pool->real_sizes[buffer_pool->read_index]
    );

    buffer_pool->read_index++;
    buffer_pool->read_index %= buffer_pool->buffer_count;
    
    sem_post(&(buffer_pool->empty));
    pthread_mutex_unlock(&(buffer_pool->lock));
    
    return read_size;
}


void write_buffer(MutexBufferPool* buffer_pool, char* src, size_t size)
{
    sem_wait(&(buffer_pool->empty));
    pthread_mutex_lock(&(buffer_pool->lock));
    
    memcpy(
        buffer_pool->buffer_pointer + buffer_pool->write_index * buffer_pool->buffer_size, 
        src,
        buffer_pool->real_sizes[buffer_pool->write_index] = size
    );

    buffer_pool->write_index++;
    buffer_pool->write_index %= buffer_pool->buffer_count;
    
    sem_post(&(buffer_pool->full));
    pthread_mutex_unlock(&(buffer_pool->lock));
}