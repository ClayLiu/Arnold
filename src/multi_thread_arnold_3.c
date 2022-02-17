#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "arnold_kernel.h"
#include "mutex_buffer_pool.h"
#include "pooling_arnold.h"

#define KB (1024)
#define MB (KB * KB)

const unsigned int init_kernel_size = 256;
const unsigned int single_buffer_size = 4 * MB;
const unsigned int buffer_count = 32;

pthread_mutex_t lock;       // 读写互斥锁
char read_write_flag = 0;   // 读写转换标志 0 means last time is reading,

_off64_t last_reading_pointer = 0;  // 上一次读后的读指针
_off64_t last_writing_pointer = 0;  // 上一次写后的读指针

MutexBufferPool* buffer_pool_rp = NULL; // 读-处理线程的缓冲池
MutexBufferPool* buffer_pool_pw = NULL; // 处理-写线程的缓冲池

void* reading_thread_kernel(void* argv)
{
    size_t read = 0;
    FILE* fp = (FILE*)argv;
    char* reading_buffer = malloc(sizeof(char) * single_buffer_size);
    if(reading_buffer == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in reading_thread.\n");
        exit(1);
    }

    do
    {
        pthread_mutex_lock(&lock);

        if(read_write_flag == 1)    // 如果上一次 IO 是写，那么先设置读指针
        {
            fseeko64(fp, last_reading_pointer, SEEK_SET);
            read_write_flag = 0;
        }

        read = fread(
            reading_buffer,
            sizeof(char),
            single_buffer_size,
            fp
        );

        fflush(fp);

        last_reading_pointer += read;

        pthread_mutex_unlock(&lock);

        write_buffer(buffer_pool_rp, reading_buffer, read);
    } while(read >= single_buffer_size);
    free(reading_buffer);
    // printf("done reading\n");
}


void* processing_thread_kernel(void* argv)
{
    size_t processed = 0;
    size_t to_process_size = 0;

    char* processing_buffer = malloc(sizeof(char) * single_buffer_size);
    if(processing_buffer == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in processing_thread.\n");
        exit(1);
    }

    ThreadPool* pool = create_pool(single_buffer_size, init_kernel_size, processing_buffer);

    do
    {
        to_process_size = read_buffer(buffer_pool_rp, processing_buffer);

        if(to_process_size == single_buffer_size)
        {
            sent_start_to_pool(pool);
            wait_pool(pool);
        }
        else
            arnold_ize_any_size_using_map(processing_buffer, init_kernel_size, to_process_size);

        write_buffer(buffer_pool_pw, processing_buffer, to_process_size);
        processed = to_process_size;
    } while(processed >= single_buffer_size);

    free(processing_buffer);
    destroy_pool(pool);

    // printf("done processing.\n");
}


void* writing_thread_kernel(void* argv)
{
    size_t wrote = 0;
    size_t to_write_size = 0;
    FILE* fp = (FILE*)argv;

    char* writing_buffer = malloc(sizeof(char) * single_buffer_size);
    if(writing_buffer == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in writing_thread.\n");
        exit(1);
    }

    do
    {
        to_write_size = read_buffer(buffer_pool_pw, writing_buffer);

        pthread_mutex_lock(&lock);

        if(read_write_flag == 0)    // 如果上一次 IO 是读，那么先设置写指针
        {
            fseeko64(fp, last_writing_pointer, SEEK_SET);
            read_write_flag = 1;
        }

        wrote = fwrite(
            writing_buffer,
            sizeof(char),
            to_write_size,
            fp
        );

        fflush(fp);

        last_writing_pointer += wrote;

        pthread_mutex_unlock(&lock);

    } while(wrote >= single_buffer_size);

    free(writing_buffer);
    // printf("done writing\n");
}


int main(int argc, char** argv)
{
    clock_t start, end;
    char* path = NULL;

    pthread_t reading_thread;
    pthread_t processing_thread;
    pthread_t writing_thread;

   if(argc == 1)   // 没有命令行参数
       return 1;

   path = argv[1];
    // path = "F:\\debug\\2.mp4";
    printf("dealing %s\n", path);
    FILE* fp = fopen(path, "rb+");

    start = clock();

    buffer_pool_rp = new_mutex_buffer_pool(
        buffer_count,
        single_buffer_size
    );

    buffer_pool_pw = new_mutex_buffer_pool(
        buffer_count,
        single_buffer_size
    );

    pthread_mutex_init(&lock, NULL);

    pthread_create(&reading_thread, NULL, reading_thread_kernel, fp);
    pthread_create(&processing_thread, NULL, processing_thread_kernel, NULL);
    pthread_create(&writing_thread, NULL, writing_thread_kernel, fp);

    pthread_join(reading_thread, NULL);
    pthread_join(processing_thread, NULL);
    pthread_join(writing_thread, NULL);

    fclose(fp);

    destroy_mutex_buffer_pool(buffer_pool_rp);
    destroy_mutex_buffer_pool(buffer_pool_pw);

    end = clock();
    printf("time used %d ms\n", end - start);
    printf("done %lld B\n", last_writing_pointer);

    return 0;
}
