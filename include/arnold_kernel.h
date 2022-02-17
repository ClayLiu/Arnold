#ifndef ARNOLD_KERNEL_HDIFHD
#define ARNOLD_KERNEL_HDIFHD

typedef int to_index_t;

typedef struct arnold_map_t
{
    unsigned int kernel_size;
    to_index_t* map;
}ArnoldMap;

ArnoldMap* build_map(unsigned int kernel_size);

void destroy_map(ArnoldMap* map);

// 使用映射的 arnold 变换
char* arnold_ize_using_map(char* array, char* temp, ArnoldMap* map);

// 使用映射的递归实现任意大小的 arnold 变换
void arnold_ize_any_size_using_map(char* data, unsigned int init_kernel_size, unsigned int data_size);

// arnold 变换
char* arnold_ize(char* array, char* temp, unsigned int kernel_size);

// 递归实现任意大小的 arnold 变换
void arnold_ize_any_size(char* data, unsigned int init_kernel_size, unsigned int data_size);

#endif