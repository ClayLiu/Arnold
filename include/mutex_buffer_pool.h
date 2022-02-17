#ifndef MUTEX_BUFFER_POOL_HDIFHD
#define MUTEX_BUFFER_POOL_HDIFHD

#include <pthread.h>
#include <semaphore.h>

typedef struct mutex_buffer_pool_t
{
    unsigned int buffer_count;
    unsigned int read_index; 
    unsigned int write_index;
    size_t buffer_size;
    size_t* real_sizes;     // 每格缓冲格中有效数据的大小
    char* buffer_pointer;

    pthread_mutex_t lock;
    sem_t empty, full;
}MutexBufferPool;

MutexBufferPool* new_mutex_buffer_pool(unsigned int buffer_count, size_t buffer_size);

void destroy_mutex_buffer_pool(MutexBufferPool* buffer_pool);

size_t read_buffer(MutexBufferPool* buffer_pool, char* dst);

void write_buffer(MutexBufferPool* buffer_pool, char* src, size_t size);

#endif