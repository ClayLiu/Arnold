#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arnold_kernel.h"

static unsigned int size_to_cycle(unsigned int kernel_size)
{
    
    if(kernel_size == 2)
        return 3;
    else
        return 3 * (kernel_size >> 2);
}


ArnoldMap* build_map(unsigned int kernel_size)
{
    int i, j, index;
    int to_i, to_j, to_index;

    unsigned int block_size = kernel_size * kernel_size;
    ArnoldMap* map = malloc(sizeof(ArnoldMap));
    map->kernel_size = kernel_size;
    map->map = malloc(sizeof(to_index_t) * block_size);
    if(map->map == NULL)
    {
        fprintf(stderr, "(Error)Memory failed in build_map on map->map.\n");
        exit(1);
    }

    for(i = 0; i < kernel_size; i++)
        for(j = 0; j < kernel_size; j++)
            {
                to_i = (i + j) % kernel_size;
                to_j = (i + 2 * j) % kernel_size;
             
                index = i * kernel_size + j;
                map->map[index] = to_i * kernel_size + to_j;
            }

    return map;
}


void destroy_map(ArnoldMap* map)
{
    free(map->map);
    map->map = NULL;
    free(map);
}


char* arnold_ize_using_map(char* array, char* temp, ArnoldMap* map)
{
    int i, k;
    unsigned int kernel_size = map->kernel_size;
    unsigned int block_size = kernel_size * kernel_size;
    
    int half_cycle = size_to_cycle(kernel_size) >> 1;
    char* p_for_swap = NULL; 
    for(k = 0; k < half_cycle; k++)
    {
        for(i = 0; i < block_size; i++)
            temp[map->map[i]] = array[i];
        
        p_for_swap = array;
        array = temp;
        temp = p_for_swap;
    }

    return array;
}


void arnold_ize_any_size_using_map(char* data, unsigned int init_kernel_size, unsigned int data_size)
{
    if(init_kernel_size <= 4)
        return;
    ArnoldMap* map = build_map(init_kernel_size);
    unsigned int block_size = init_kernel_size * init_kernel_size;
    char* temp = malloc(sizeof(char) * init_kernel_size * init_kernel_size);
    char* done_pointer = NULL;

    while(data_size >= block_size)
    {
        // done_pointer = arnold_ize(data, temp, init_kernel_size);
        done_pointer = arnold_ize_using_map(data, temp, map);
        if(done_pointer != data)
            memcpy(data, done_pointer, block_size);

        data += block_size;
        data_size -= block_size;
    }

    free(temp);
    destroy_map(map);
    arnold_ize_any_size_using_map(data, init_kernel_size >> 1, data_size);
}


char* arnold_ize(char* array, char* temp, unsigned int kernel_size)
{
    int k;
    int i, j;
    int to_i, to_j;

    int half_cycle = size_to_cycle(kernel_size) >> 1;
    char* p_for_swap = NULL; 
    for(k = 0; k < half_cycle; k++)
    {
        for(i = 0; i < kernel_size; i++)
            for(j = 0; j < kernel_size; j++)
            {
                to_i = (i + j) % kernel_size;
                to_j = (i + 2 * j) % kernel_size;

                temp[to_i * kernel_size + to_j] = array[i * kernel_size + j];
            }

        p_for_swap = array;
        array = temp;
        temp = p_for_swap;
    }

    return array;
}

void arnold_ize_any_size(char* data, unsigned int init_kernel_size, unsigned int data_size)
{
    if(init_kernel_size <= 4)
        return;

    unsigned int block_size = init_kernel_size * init_kernel_size;
    char* temp = malloc(sizeof(char) * init_kernel_size * init_kernel_size);
    char* done_pointer = NULL;

    while(data_size >= block_size)
    {
        done_pointer = arnold_ize(data, temp, init_kernel_size);
        if(done_pointer != data)
            memcpy(data, done_pointer, block_size);

        data += block_size;
        data_size -= block_size;
    }

    free(temp);
    arnold_ize_any_size(data, init_kernel_size >> 1, data_size);
}