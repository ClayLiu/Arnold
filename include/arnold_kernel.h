#ifndef ARNOLD_KERNEL_HDIFHD
#define ARNOLD_KERNEL_HDIFHD

enum block_size_value 
{
    block_size_256   = 1u << (8u << 1),  // 2 ^ 16
    block_size_128   = 1u << (7u << 1),  // 2 ^ 14
    block_size_64    = 1u << (6u << 1),  // 2 ^ 12
    block_size_32    = 1u << (5u << 1),  // 2 ^ 10
    block_size_16    = 1u << (4u << 1),  // 2 ^ 8
    block_size_08    = 1u << (3u << 1),  // 2 ^ 6
};

/* 对任意大小的数据块进行半个周期的 arnold 变换 */
void arnold_ize_half_cycle_for_any_size(char* array, unsigned int data_size);

/* 对任意保证为 block 大小的数据块进行半个周期的 arnold 变换 */
void arnold_ize_half_cycle_for_block(char* array, unsigned int kernel_size);

/* 对大小是若干个 block_size_256 的数据块进行半个周期的 arnold 变换 */
void arnold_ize_half_cycle_for_chunk(char* array, unsigned int chunk_size);

#endif