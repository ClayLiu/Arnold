/* This file is wrote to build arnold_kernel.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/arnold_kernel.h"

#define swap(x, y) ((x) ^= (y) ^= (x) ^= (y))

static unsigned int size_to_cycle(unsigned int kernel_size)
{
    if(kernel_size == 2)
        return 3;
    else
        return 3 * (kernel_size >> 2);
}

// void swap(char* a, char* b)
// {
//     char t;
//     t = *a;
//     *a = *b;
//     *b = t;
// }

void arnold_ize_using_map_one_time(int* array, int* temp, ArnoldMap* map)
{
    int i;
    unsigned int kernel_size = map->kernel_size;
    unsigned int block_size = kernel_size * kernel_size;

    for(i = 0; i < block_size; i++)
        temp[map->map[i]] = array[i];
    
    memcpy(array, temp, block_size * sizeof(int));
}


void print(int N, FILE* fp)
{
    int i, j, k;
    int half_cycle = size_to_cycle(N) >> 1;

    ArnoldMap* map = build_map(N);

    int* index = malloc(sizeof(int) * N * N);
    int* temp = malloc(sizeof(int) * N * N);
    int* swaping_map = malloc(sizeof(int) * N * N);

    for(i = 0; i < N * N; i++)
        index[i] = i;

    for(k = 0; k < half_cycle; k++)
        arnold_ize_using_map_one_time(index, temp, map);

    // for(i = 0; i < N; i++)
    // {
    //     for(j = 0; j < N; j++)
    //         printf("%5d", index[i * N + j]);
        
    //     putchar('\n');
    // }

    for(i = j = 0; i < N * N; i++)
    {
        if(i < index[i])
        {
            swaping_map[j] = i;
            swaping_map[j + 1] = index[i];
            j += 2;

            // printf("%d <-> %d\n", i, index[i]);
            fprintf(fp, "    swap(array[%d], array[%d]);\n", i, index[i]);
        }
    }

    // for(i = 0; i < j; i += 2)
    // {
    //     swap(index[swaping_map[i]], index[swaping_map[i + 1]]);
    // }


    // for(i = 0; i < N; i++)
    // {
    //     for(j = 0; j < N; j++)
    //         printf("%5d", index[i * N + j]);
        
    //     putchar('\n');
    // }

    free(index);
    free(temp);
    free(swaping_map);
    destroy_map(map);
}


int main()
{
    // FILE* fp = fopen("arnold_kernel.c", "w");
    // fprintf(fp, "123");
    // fclose(fp);
    FILE* fp = fopen("arnold_kernel.c", "w");
    int N;

    for(N = 8; N <= 256; N <<= 1)
    {
        fprintf(fp, "static void arnold_ize_%d(char* array)\n{\n", N);
        print(N, fp);
        fprintf(fp, "}\n\n\n");
    }


    fclose(fp);
    return 0;
}