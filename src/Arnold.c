#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "arnold_kernel.h"

int size_to_cycle(int size)
{
    if(size == 2)
        return 3;
    else
        return 3 * (size >> 2);
}


// char* arnold_ize(char* array, char* temp, int kernel_size)
// {
//     int k;
//     int i, j;
//     int to_i, to_j;

//     int half_cycle = size_to_cycle(kernel_size) >> 1;
//     char* p_for_swap = NULL; 
//     for(k = 0; k < half_cycle; k++)
//     {
//         for(i = 0; i < kernel_size; i++)
//             for(j = 0; j < kernel_size; j++)
//             {
//                 to_i = (i + j) % kernel_size;
//                 to_j = (i + 2 * j) % kernel_size;

//                 temp[to_i * kernel_size + to_j] = array[i * kernel_size + j];
//             }

//         p_for_swap = array;
//         array = temp;
//         temp = p_for_swap;
//     }

//     return array;
// }


unsigned long long processing(FILE* fp, int kernel_size)
{
    unsigned int i = 0;
    unsigned int block_size = kernel_size * kernel_size;
    int read;
    char* buffer = NULL;
    char* copy_buffer = NULL;
    char* done_pointer = NULL;
    ArnoldMap* map = build_map(kernel_size);

    if(kernel_size == 4)    // 2021-6-9 23:15:16 这里要修改，该为 8 时程序置乱可恢复，4却不行。。   
        return 0;           // 2021-6-10 15:56:02 解决

    buffer = malloc(block_size * sizeof(char));
    copy_buffer = malloc(block_size * sizeof(char));

    while(
        read = fread(buffer, sizeof(char), block_size, fp),
        read == block_size
    )
    {
        // done_pointer = arnold_ize(buffer, copy_buffer, kernel_size);
        done_pointer = arnold_ize_using_map(buffer, copy_buffer, map);

        fseek(fp, - block_size, SEEK_CUR);

        fwrite(done_pointer, sizeof(char), block_size, fp);
        
        fflush(fp);                 // 即为下句的主要效果 2021-6-10 20:03:04
        // fseek(fp, 0, SEEK_CUR);  // 因为读写转换问题，写完之后要设置一下指针偏移 2021-6-10 15:53:39 According to KHJJ
        i++;
    }

    free(buffer);
    free(copy_buffer);
    
    fseek(fp, - read, SEEK_END);
    destroy_map(map);

    return ((unsigned long long)i * block_size) + processing(fp, kernel_size >> 1);
}


int main(int argc, char** argv)
{
    clock_t start, end;
    char* path = NULL;
    FILE* fp = NULL;
    unsigned long long done_size;

    if(argc == 1)
        return 1;

    path = argv[1];
    printf("dealing %s\n", path);
    
    fp = fopen(path, "rb+");

    start = clock();
    
    done_size = processing(fp, 256);
    fclose(fp);
    
    end = clock();

    printf("time used %d ms\n", end - start);
    printf("done %lld B", done_size);
    return 0;
}