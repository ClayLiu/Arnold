#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "arnold_kernel.h"
// #include "mutex_buffer_pool.h"
// #include "pooling_arnold.h"

#define KB (1024)
#define MB (KB * KB)

const size_t single_buffer_size = 4 * MB;
// const unsigned int buffer_count = 32;
#define buffer_count 32u

pthread_mutex_t lock;       // 读写互斥锁
char read_write_flag = 0;   // 读写转换标志
enum read_write_flag_value {reading_flag, writing_flag};

_off64_t last_reading_pointer = 0;  // 上一次读后的读指针
_off64_t last_writing_pointer = 0;  // 上一次写后的读指针

char* buffer_array[buffer_count];   // 缓冲区
size_t real_size[buffer_count];     // 每格缓冲区实际有数据大小

sem_t can_read[buffer_count];
sem_t can_process[buffer_count];
sem_t can_write[buffer_count];

void* reading_thread_kernel(void* argv)
{
    size_t read = 0;
    FILE* fp = (FILE*)argv;
    unsigned int buffer_index = 0;
    char* reading_buffer = NULL;

    do
    {
        pthread_mutex_lock(&lock);
        sem_wait(can_read + buffer_index);

        reading_buffer = buffer_array[buffer_index];

        if(read_write_flag == writing_flag)    // 如果上一次 IO 是写，那么先设置读指针
        {
            fflush(fp);
            fseeko64(fp, last_reading_pointer, SEEK_SET);
            read_write_flag = reading_flag;
        }

        read = fread(
            reading_buffer,
            sizeof(char),
            single_buffer_size,
            fp
        );
        real_size[buffer_index] = read;
        last_reading_pointer += read;
        // printf("read = %llu\n", read);

        sem_post(can_process + buffer_index);
        pthread_mutex_unlock(&lock);

        buffer_index++;
        buffer_index %= buffer_count;
    } while(read >= single_buffer_size);
    printf("done reading\n");
}


void* processing_thread_kernel(void* argv)
{
    size_t processed = 0;
    size_t to_process_size = 0;
    unsigned int buffer_index = 0;
    char* processing_buffer = NULL;

    do
    {
        sem_wait(can_process + buffer_index);
        processing_buffer = buffer_array[buffer_index];
        to_process_size = real_size[buffer_index];

        if(to_process_size == single_buffer_size)
            arnold_ize_half_cycle_for_chunk(processing_buffer, to_process_size);
        else
            arnold_ize_half_cycle_for_any_size(processing_buffer, to_process_size);

        processed = to_process_size;
        sem_post(can_write + buffer_index);
        buffer_index++;
        buffer_index %= buffer_count;
    } while(processed >= single_buffer_size);

    // printf("done processing.\n");
}


void* writing_thread_kernel(void* argv)
{
    size_t wrote = 0;
    FILE* fp = (FILE*)argv;
    unsigned int buffer_index = 0;
    char* writing_buffer = NULL;

    do
    {
        pthread_mutex_lock(&lock);
        sem_wait(can_write + buffer_index);
        writing_buffer = buffer_array[buffer_index];

        if(read_write_flag == reading_flag)    // 如果上一次 IO 是读，那么先设置写指针
        {
            fflush(fp);
            fseeko64(fp, last_writing_pointer, SEEK_SET);
            read_write_flag = writing_flag;
        }

        wrote = fwrite(
            writing_buffer,
            sizeof(char),
            real_size[buffer_index],
            fp
        );

        last_writing_pointer += wrote;
        
        sem_post(can_read + buffer_index);
        pthread_mutex_unlock(&lock);
        buffer_index++;
        buffer_index %= buffer_count;
    } while(wrote >= single_buffer_size);
    // printf("done writing\n");
}


int main(int argc, char** argv)
{
    unsigned int i;
    clock_t start, end;
    char* path = NULL;

    pthread_t reading_thread;
    pthread_t processing_thread;
    pthread_t writing_thread;

    if(argc == 1)   // 没有命令行参数
    {
        printf("fatal error: no input files");
        return 1;
    }

    path = argv[1];
    // path = "test\\CS1.6.rar";
    printf("dealing %s\n", path);
    FILE* fp = fopen(path, "rb+");

    start = clock();

    for(i = 0; i < buffer_count; i++)
    {
        buffer_array[i] = malloc(single_buffer_size);
        if(buffer_array[i] == NULL)
        {
            fprintf(stderr, "fatal error: no more memory in newing buffer[%u]", i);
            
            while(--i >= 0)
                free(buffer_array[i]);
            
            return 2;
        }
    }

    for(i = 0; i < buffer_count; i++)
    {
        sem_init(can_read + i, 0, 1);
        sem_init(can_process + i, 0, 0);
        sem_init(can_write + i, 0, 0);
    }

    pthread_mutex_init(&lock, NULL);

    pthread_create(&reading_thread, NULL, reading_thread_kernel, fp);
    pthread_create(&processing_thread, NULL, processing_thread_kernel, NULL);
    pthread_create(&writing_thread, NULL, writing_thread_kernel, fp);

    pthread_join(reading_thread, NULL);
    pthread_join(processing_thread, NULL);
    pthread_join(writing_thread, NULL);

    fclose(fp);

    for(i = 0; i < buffer_count; i++)
        free(buffer_array[i]);

    for(i = 0; i < buffer_count; i++)
    {
        sem_destroy(can_read + i);
        sem_destroy(can_process + i);
        sem_destroy(can_write + i);
    }

    end = clock();
    printf("time used %d ms\n", end - start);
    printf("done %llu B\n", last_writing_pointer);

    return 0;
}
