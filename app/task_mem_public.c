#include "task.h"

// 全局动态内存终端测试.
/*
终端测试1:
add malloc memtest	// 申请 / 更新全局动态内存
add show			// 读取指定的动态内存
add show free		// 释放动态内存

终端测试2:
add malloc memtest	// 申请 / 更新全局动态内存
add show			// 读取指定的动态内存
del malloc			// 删除相关任务, 会自动释放动态

终端测试3:
add mem0 20
add mem1 35
add malloc memtest
add show
del mem0			// 释放 mem0
sort				// 执行内存整理

终端测试4:
add mem0 1
add mem1 2
add malloc memtest
add mem2 19
del mem1
add mem1 20
del mem2
add show
sort

终端测试5:
add mem0 1
add mem1 2
add mem2 3
add malloc memtest
del mem0
del mem2
add mem0 21
add mem2 22
del mem0
add show
add mem0 60
*/

ezsm_t task_malloc(ezsm_t s, void *p) {
    char *mem_ptr;  // 每次重入, 都要必须重新读取动态内存指针. 1. 这是个局部变量. 2. 动态内存的指针可能因为优化而改变.
    switch (s) {
        case EZSM_INIT: {  // 申请一个全局动态内存, 并赋值
            int len = strlen(p) + 5;
            mem_ptr = self_malloc(5 + strlen(p));  // 申请动态内存
            snprintf(mem_ptr, len, "%02d, %s", s, (char *)p);
            if (mem_ptr == NULL) return EZSM_ERROR;
            printf("task_malloc mem success\n");
            return ++s;
        }
        default:
            mem_ptr = self_mem_get();
            if (mem_ptr == NULL) return EZSM_ERROR;
            mem_ptr[0] = '0' + s / 10;  // 每秒更新一次 "%02d"
            mem_ptr[1] = '0' + s % 10;
            ezos_delay(1000);
            if (++s > 99) s = 1;
            return s;
        case EZSM_ERROR:
            printf("task_malloc failed to get mem ptr\n");
            return EZSM_DONE;
    }
}

ezsm_t task_show(ezsm_t s, void *p) {           // 该任务使用全局动态内存, 并最后释放它.
    char *mem_ptr = ezos_mem_get(TASK_MALLOC);  // 获取全局变量
    if (mem_ptr == NULL) {
        printf("task_show failed to get mem ptr\n");
        return EZSM_DONE;
    } else {
        printf("task_show: %s\n", mem_ptr);
        if (!strcmp("free", (char *)p)) {
            ezos_free(TASK_MALLOC);  // 释放指定动态内存
        }
        ezos_delay(1000);
        return s;
    }
}
