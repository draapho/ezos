/**
 * \file            ezos_cfg.h
 * \brief           ezos config header file
 */

/*
 * Copyright (c) 2022 draapho
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          draapho <draapo@gmail.com>
 * Date:            2022/10/18
 */

#ifndef EZOS_CFG_H__
#define EZOS_CFG_H__

#include <stdint.h>
typedef int16_t ezsm_t;  // State Machine, 任务状态机类型, 必须是有符号类型.

/* ezos config */
#define EZOS_TICK_MS 1   // ezos_tick_irq() 调用周期决定ezos的系统时钟, 建议值 1-10ms.
#define EZOS_TASK_MAX 8  // 设置ezos可运行任务数组的最大值. 推荐值: 8-32. 该值只影响内存大小, 不会影响调度性能.

#define EZOS_SEM    // 使能ezos信号量
#define EZOS_MUTEX  // 使能ezos互斥量
#define EZOS_EVENT  // 使能ezos事件
#define EZOS_MQ     // 使能ezos消息队列, Message Queue

#define EZOS_MEM               // 使能ezos动态内存
#define EZOS_MEM_BLK_NUM 8     // 动态内存块数量, 最大值 254
#define EZOS_MEM_BLK_SIZE 16U  // 动态内存块大小
#define EZOS_MEM_POOL_SIZE (EZOS_MEM_BLK_NUM * EZOS_MEM_BLK_SIZE)
// 注意: 启用内存整理, 任务切换后, 获得的内存指针地址就可能发生变化, 必须重新读取内存指针.
#define EZOS_MEM_SORT 2  // 内存整理. 0-禁用, 1-仅手动整理, 2-仅申请失败时整理, 3-闲时自动整理, 默认值: 2
#define EZOS_MEM_SHOW    // 内存整理时, 显示内存情况. 仅用于观察调试. 需要使能 EZOS_LOG.

// #define EZOS_TEST  // 使能ezos性能测试

#define EZOS_ASSERT        // 使能断言功能
#define EZOS_ASSERT_FUN 0  // 定义断言函数. 0-进入死循环, 1-ez_printf, 默认值: 0

#define EZOS_LOG          // 使能调试打印功能
#define EZOS_LOG_LEVEL 3  // 定义调试级别. 0-ERROR, 1-DEBUG, 2-WARNING, 3-INFO, 默认值: 3

/* X Macro: 请将任务按优先级从高到低进行X宏定义 */
/* 最后的字符串用于终端指令, 不用终端可以全部省略 */
/* 使能 EZOS_MEM 支持动态内存时, 最多253个任务 */
#define EZOS_TASKS_NAME_FUN_STR           \
    X(TASK_SHELL, task_shell, "")         \
    X(TASK_LED, task_led, "led")          \
    X(TASK_HELLO, task_hello, "hello")    \
    X(TASK_HELLO_1, task_hello, "")       \
    X(TASK_HSM, task_hsm, "hsm")          \
    X(TASK_IPC, task_ipc, "ipc")          \
    X(TASK_WAIT_SEM, task_sem, "sem")     \
    X(TASK_WAIT_MQ, task_mq, "mq")        \
    X(TASK_MEM0, task_mems, "mem0")       \
    X(TASK_MEM1, task_mems, "mem1")       \
    X(TASK_MEM2, task_mems, "mem2")       \
    X(TASK_MALLOC, task_malloc, "malloc") \
    X(TASK_SHOW, task_show, "show")

#endif /* EZOS_CFG_H__ */
