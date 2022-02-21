#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "arnold_kernel.h"


unsigned long long processing(FILE* fp, int kernel_size)
{
    if(kernel_size == 4)   
        return 0;          

    unsigned int i = 0;
    unsigned int block_size = kernel_size * kernel_size;
    int read;
    char* buffer = NULL;

    buffer = malloc(block_size * sizeof(char));

    while(
        read = fread(buffer, sizeof(char), block_size, fp),
        read == block_size
    )
    {
        arnold_ize_half_cycle_for_block(buffer, kernel_size);
        fseek(fp, - block_size, SEEK_CUR);
        fwrite(buffer, sizeof(char), block_size, fp);
        
        fflush(fp);                 // 即为下句的主要效果 2021-6-10 20:03:04
        // fseek(fp, 0, SEEK_CUR);  // 因为读写转换问题，写完之后要设置一下指针偏移 2021-6-10 15:53:39 According to KHJJ
        i++;
    }

    free(buffer);
    fseek(fp, - read, SEEK_END);

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
    printf("done %llu B", done_size);
    return 0;
}