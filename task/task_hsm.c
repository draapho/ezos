#include "task.h"

// 多层状态机范例.
// 终端内调用 "add hsm", 顺利执行任务;
// 终端内调用 "add hsm x", 会有错误处理.

ezsm_t sub_task_1(ezsm_t s, void *p);
ezsm_t sub_task_2(ezsm_t s, void *p);
ezsm_t sub_task_21(ezsm_t s, void *p);

ezsm_t sub_task_1(ezsm_t s, void *p) {
    switch (s) {
        case EZSM_INIT:
            printf("  sub_task_1 init\r\n");
            ezos_delay(1000);
            return ++s;
        case 1:
            printf("  sub_task_1 do something\r\n");
            ezos_delay(1000);
            return EZSM_DEINIT;
        case EZSM_DEINIT:
            printf("  sub_task_1 deinit\r\n");
            // return EZSM_DONE;
        case EZSM_ERROR:
        default:
            return EZSM_DONE;
    }
}

ezsm_t sub_task_2(ezsm_t s, void *p) {
    static ezsm_t ss;
    switch (s) {
        case EZSM_INIT:
            printf("  sub_task_2 init\r\n");
            ezos_delay(1000);
            return ++s;
        case 1:
            printf("  sub_task_2 call sub_task_21\r\n");
            ezos_delay(100);
            ss = 0;
            return ++s;
        case 2:
            ss = sub_task_21(ss, p);
            if (ss == EZSM_DONE) {  // sub_task_21 子任务完成
                ezos_delay(1000);
                return EZSM_DEINIT;
            } else if (ss == EZSM_ERROR) {  // sub_task_21 子任务错误处理
                ezos_delay_awhile();
                printf("  sub_task_2 error\r\n");
                return EZSM_ERROR;
            } else {
                return s;  // 继续执行 sub_task_21 子任务
            }
        case EZSM_DEINIT:
            printf("  sub_task_2 deinit\r\n");
            return EZSM_DONE;
        case EZSM_ERROR:  // task_hsm 中提前处理了 EZSM_ERROR 状态, 所以不会运行到这个状态.
        default:
            printf("  sub_task_2 impossible state\r\n");
            return EZSM_DONE;
    }
}

ezsm_t sub_task_21(ezsm_t s, void *p) {
    switch (s) {
        case EZSM_INIT:
            printf("    sub_task_21 init\r\n");
            ezos_delay(1000);
            return ++s;
        case 1:
            printf("    sub_task_21 do something\r\n");
            ezos_delay(1000);
            if (p == NULL) {
                return EZSM_DEINIT;
            } else {
                printf("    sub_task_21 error\r\n");
                return EZSM_ERROR;
            }
        case EZSM_DEINIT:
            printf("    sub_task_21 deinit\r\n");
            return EZSM_DONE;
        case EZSM_ERROR:  // sub_task_2 中提前处理了 EZSM_ERROR 状态, 所以不会运行到这个状态.
        default:
            printf("    sub_task_21 impossible state\r\n");
            return EZSM_DONE;
    }
}

// 多层状态机. 终端内调用 "add hsm", 顺利执行任务; "add hsm xxx", 会有错误处理.
ezsm_t task_hsm(ezsm_t s, void *p) {
    static ezsm_t ss;  // 此处必须是 static, 协作式任务不会自动保存上下文!
    switch (s) {
        case EZSM_INIT:
            printf("task_hsm init\r\n");
            printf("task_hsm call sub_task_1\r\n");
            ss = 0;
            return ++s;
        case 1:
            ss = sub_task_1(ss, NULL);
            if (ss == EZSM_DONE) {  // sub_task_1 子任务完成
                ezos_delay(1000);
                printf("task_hsm call sub_task_2\r\n");
                ss = 0;
                return ++s;
            } else {
                return s;  // 继续执行 sub_task_1 子任务
            }
        case 2:
            ss = sub_task_2(ss, p);
            if (ss == EZSM_DONE) {  // sub_task_1 子任务完成
                ezos_delay(1000);
                ss = 0;
                return EZSM_DEINIT;
            } else if (ss == EZSM_ERROR) {
                ezos_delay_awhile();
                return EZSM_ERROR;
            } else {
                return s;  // 继续执行 sub_task_2 子任务
            }
        case EZSM_DEINIT:
            printf("task_hsm deinit\r\n");
            return EZSM_DONE;  // 任务结束, 将自动删除该任务
        default:
        case EZSM_ERROR:  // 错误处理
            printf("task_hsm error\r\n");
            return EZSM_DONE;
    }
}
