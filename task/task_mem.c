#include <stdlib.h>

#include "task.h"

// 动态内存终端测试.
/*
终端测试1:
add mem0 20     // mem0 申请20个字节的内存
add mem1 64     // mem1 申请64个字节的内存
del mem0		// 释放 mem0
sort			// 整理并打印动态内存
add mem2 64		// mem2 申请动态内存.

终端测试2:
add mem0 1
add mem1 2
add mem2 3
del mem0
add mem0 24
del mem2
sort
*/

ezsm_t task_mems(ezsm_t s, void *p) {
    char *mem_ptr;  // 每次重入, 都要必须重新读取动态内存指针. 1. 这是个局部变量. 2. 动态内存的指针可能因为优化而改变.

    switch (s) {
        case EZSM_INIT: {  // 申请一个动态内存, 并赋值
            unsigned int num = atoi(p);
            const ez_task_t *cur = ezos_self_info();

            mem_ptr = self_malloc(num);  // 申请动态内存
            if (mem_ptr == NULL) return EZSM_ERROR;

            printf("mem task and index:%02x,%02x\r\n", cur->name, cur->mem);
            memset(mem_ptr, cur->name, num);  // 动态内存全部写入内存名称值
            ezos_delay(100);
            return ++s;
        }
        case 1: {  // 读取并打印当前动态内存的内容
            uint16_t size, i;
            // mem_ptr = self_mem_get();  // 读取动态内存
            mem_ptr = ezos_malloc(ezos_self_name(), 0, &size);  // 读取动态内存和大小
            if (mem_ptr == NULL) return EZSM_ERROR;

            printf("mem content:\r\n");
            for (i = 0; i < size; i++) {
                printf("%02x ", *mem_ptr++);
            }
            printf("\r\n");
            ezos_delay(100);
            return ++s;
        }
        case 2: {  // 打印出整个动态内存的详细信息
            ezos_mem_show();
            ezos_delay_forever();
            return s;
        }
        case EZSM_ERROR:
            printf("ezos_malloc failed\r\n");
        default:
            return EZSM_DONE;
    }
}
