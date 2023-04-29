#include "task.h"

// 这是单一任务多重实例化的简单范例, 直接打印传入的字符串.
// 在"ezos_cfg.h"中关联了两个任务名: TASK_HELLO 和 TASK_HELLO_1
// 这两个任务可以互不影响的同时运行, 会根据传入的字符串打印不同的信息.
// 多重实例的任务一般会有传入不同的输入参数, 需要任务变量进行差异化处理, 因而最好配合 EZOS_MEM 使用.

ezsm_t task_hello(ezsm_t s, void *p) {
    switch (s) {
        case EZSM_INIT:  // 状态 EZSM_INIT=0, 任务入口, 通常做一些任务的初始化工作
            if (p == NULL) {
                return EZSM_ERROR;
            } else {
                printf("hello, %s\n", (char *)p);
                ezos_delay(5000);
                return ++s;  // 进入下一个状态, 就是 case 1
            }
        case 1:
            printf("%s, welcome to task %d\n", (char *)p, ezos_self_name());
            ezos_delay(5000);
            return EZSM_DEINIT;
        case EZSM_DEINIT:
            printf("goodbye, %s\n", (char *)p);
            return EZSM_DONE;  // 任务结束, 将自动删除该任务
        default:
        case EZSM_ERROR:  // 错误处理
            printf("nothing to say with nameless\n");
            return EZSM_DONE;
    }
}
