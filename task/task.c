/**
 * \file            task.c
 * \brief           task source file.
 */

#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ezos_handle_callback(void) {
    // LL_IWDG_ReloadCounter(IWDG); // 复位看门狗
    if (ezos_is_done() == EZOS_OK) {
        ;  // 如有需要, 此处可进入低功耗模式
    }
}

void system_init(void) {
    ezos_init();
    __enable_irq();                            // 调用 ezos_init() 之后再打开中断.
    ezos_set_idle_hook(ezos_handle_callback);  // 设置闲时任务钩子函数

    uart_buff_init_all();
    led_init_all();
    ipc_test_init();

    task_add(TASK_SHELL);
    task_add(TASK_LED);
    ezos_add(TASK_HELLO, &"Jack");  // 同一个任务函数 task_hello, 可以有两个不同的任务实例.
    ezos_add(TASK_HELLO_1, &"Mary");
    // 更多范例请用终端命令调用, 格式 add [task_str]

    // 多层状态机范例
    // task_add(TASK_HSM);
    // ezos_add(TASK_HSM, &"x");

    // IPC信号量范例
    // task_add(TASK_WAIT_SEM);
    // task_add(TASK_IPC);
    // task_add(TASK_WAIT_MQ);
    // ezos_add(TASK_IPC, "test mq");

    ezos_schedule();
}

ezsm_t task_shell(ezsm_t s, void *p) {
    switch (s) {
        case EZSM_INIT:  // 完成相关初始化工作
            shell_init();
            ezos_delay_awhile();
            return ++s;
        case 1: {
            uint8_t buff[20];
            int32_t n;
            char *p;
            n = uart_rx(&UART_MCU, buff, sizeof(buff));
            p = (char *)buff;
            while (n-- > 0) {
                shell(*p++);
            };
            ezos_delay(10);
            return s;  // 死循环, 抓取uart端的按键输入
        }
        default:
        case EZSM_ERROR:  // 错误处理, 这里其实没用到
            LOG("task_shell err\n");
            return EZSM_DONE;
    }
}
