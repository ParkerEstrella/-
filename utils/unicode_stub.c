/* FatFs LFN Unicode 支持存根
 * 当 _USE_LFN >= 2 时需要这些函数 */

#include "common_types.h"
#include <stdlib.h>
#include <string.h>

/* FF 编码转换：存根实现（仅 ASCII） */
unsigned short ff_convert(unsigned short src, unsigned int dir)
{
    (void)dir;
    /* ASCII 范围直通 */
    if (src < 0x80) return src;
    /* 非 ASCII 字符返回空格 */
    return ' ';
}

/* FF 大写转换 */
unsigned short ff_wtoupper(unsigned short chr)
{
    if (chr >= 'a' && chr <= 'z') {
        return chr - ('a' - 'A');
    }
    return chr;
}

/* 堆内存分配（仅 _USE_LFN == 3 时需要） */
void* ff_memalloc(unsigned int msize)
{
    return malloc(msize);
}

void ff_memfree(void* mblock)
{
    free(mblock);
}
