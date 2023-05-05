/**
 * \file            ezos.h
 * \brief           ezos header file
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
 * Date:            2023/04/30 - optimize ezos_mem_sort; simplifier LOG.h;
 */
#ifndef EZOS_H__
#define EZOS_H__

#include "ezos_cfg.h"
#include "ezos_def.h"
#include "ezos_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* typedef */
typedef enum {
#define X(name, ...) name,  // 提取任务名称, 按优先级排列
    EZOS_TASKS_NAME_FUN_STR
#undef X
        EZOS_TASK_IDLE,  // 系统空闲任务为最低优先级
    EZOS_TASK_NAME_END,  // 系统任务列表数量
    EZOS_TASK_NAME_MAX = UINT8_MAX,
} task_name_t;

struct ez_task {           // ezos 任务类型定义
    struct ez_task* next;  // 任务链表指针
    void* para;            // 参数指针
#ifdef EZOS_IPC
    void* ipc;  // 任务相关IPC
#endif
    eztm_t delay;        // 任务等待时间
    ezsm_t state;        // 任务状态机
    task_name_t name;    // 任务名称和优先级
    ez_status_t status;  // 任务状态
#ifdef EZOS_MEM
    uint8_t mem;  // 任务相关的动态内存表位置
#endif
};
typedef struct ez_task ez_task_t;

#ifdef EZOS_TEST
typedef struct {             // 性能测试类型定义
    uint32_t idle_gap_tick;  // 系统空闲任务调用间隔计数
    uint32_t task_run_tick;  // 用户任务调用时间计数
} ez_test_tick_t;

typedef struct {                // 性能测试类型定义
    uint32_t idle_gap_cur;      // 系统空闲任务调用间隔当前值
    uint32_t idle_gap_max;      // 系统空闲任务调用间隔最大值
    uint32_t task_run_cur;      // 用户任务调用时间当前值
    uint32_t task_run_max;      // 用户任务调用时间最大值 (包括中断占用的时间)
    task_name_t task_run_name;  // 用户任务名称
    ezsm_t task_run_state;      // 用户任务最坏情况所在状态
    uint8_t trash_num;          // 回收链表节点数当前值
    uint8_t trash_num_min;      // 回收链表节点数最小值
} ez_test_t;
#endif /* EZOS_TEST */

/* task define */
#define EZOS_BEGIN(s) \
    switch (s) {      \
        case EZSM_INIT:

#define EZOS_YIELD(ms) \
    ezos_delay(ms);    \
    return __LINE__;   \
    case __LINE__:

#define EZOS_YIELD_UNTIL(ms, cond) \
    ezos_delay(ms);                \
    return (cond) ? __LINE__ : s;  \
    case __LINE__:

#define EZOS_GOTO_BEGIN() \
    return EZSM_INIT;

#define EZOS_GOTO_END() \
    return EZSM_DONE;

#define EZOS_END()        \
    default:              \
        return EZSM_DONE; \
        }

/* generate task function */
#define X(name, task, ...) ezsm_t(task)(ezsm_t s, void* p);  // 生成任务函数
EZOS_TASKS_NAME_FUN_STR
#undef X

/* function */
void ezos_init(void);                          // ezos 初始化函数, 必须在定时中断之前调用
void ezos_tick_irq(void);                      // ezos 系统时钟定时扫描. 在定时中断中调用
void ezos_set_idle_hook(void (*hook)(void));   // 设置空闲任务钩子函数
void ezos_set_sleep_hook(void (*hook)(void));  // 设置系统睡眠钩子函数
void ezos_schedule(void);                      // 任务调度, main函数的最后调用

ez_err_t ezos_add(task_name_t name, void* para);  // 添加任务
ez_err_t ezos_delay(eztm_t time_ms);              // 当前任务等待, 单位ms
ez_err_t ezos_delete(task_name_t name);           // 删除任务, 用户一般不使用, 而是让任务运行完后自动删除
ez_err_t ezos_done(void);                      // 判断所有任务是否执行完成, 可用于辅助判断系统是否可以进入睡眠状态
ez_status_t ezos_status(task_name_t name);    // 获取任务状态
task_name_t ezos_self_name(void);                 // 获取当前任务的名称
const ez_task_t* ezos_self_info(void);            // 获取当前任务的指针

ez_err_t ezos_resume_irq(task_name_t name);  // 中断中恢复指定任务
ez_err_t ezos_resume(task_name_t name);      // 恢复指定任务
ez_err_t ezos_frozen(task_name_t name);      // 冻结指定任务

void delay_us(uint32_t us);  // 阻塞式毫秒延时函数

// ezos_add 的简化函数
__STATIC_FORCEINLINE ez_err_t task_add(task_name_t name) {
    return ezos_add(name, NULL);
}
__STATIC_FORCEINLINE ez_err_t force_add(task_name_t name, void* para) {
    ezos_delete(name);
    return ezos_add(name, para);
}

// ezos_delay 的简化函数
__STATIC_FORCEINLINE ez_err_t ezos_delay_null(void) {
    return ezos_delay(EZTM_NULL);  // 不延时, 等待轮询高优先级任务
}
__STATIC_FORCEINLINE ez_err_t ezos_delay_awhile(void) {
    return ezos_delay(EZTM_AWHILE);  // 延时一会, 等待轮询所有任务
}
__STATIC_FORCEINLINE ez_err_t ezos_delay_forever(void) {
    return ezos_delay(EZTM_FOREVER);  // 永久挂起, 任务冻结状态
}

#ifdef EZOS_TEST
void ezos_idle_gap_tick_increase(void);     // 用户决定调用周期, 建议使用1ms周期
void ezos_task_run_tick_increase(void);     // 用户决定调用周期, 建议使用1ms周期
ez_test_t ezos_get_test_performance(void);  // 获取ezos性能测试数据
#endif

#ifdef EZOS_SEM
ez_err_t ezos_sem_init(ez_sem_t* sem, uint8_t value);   // 初始化信号量
ez_err_t ezos_sem_take(ez_sem_t* sem, eztm_t timeout);  // 获取信号量
ez_err_t ezos_sem_release(ez_sem_t* sem);               // 释放信号量
ez_err_t ezos_sem_release_irq(ez_sem_t* sem);           // 中断用释放信号量
#endif

#ifdef EZOS_MUTEX
ez_err_t ezos_mutex_init(ez_mutex_t* mutex, uint8_t value);   // 初始化互斥量
ez_err_t ezos_mutex_take(ez_mutex_t* mutex, eztm_t timeout);  // 获取互斥量
ez_err_t ezos_mutex_release(ez_mutex_t* mutex);               // 释放互斥量
ez_err_t ezos_mutex_release_irq(ez_mutex_t* mutex);           // 中断用释放互斥量
#endif

#ifdef EZOS_EVENT
ez_err_t ezos_event_init(ez_event_t* event, uint32_t value);                                      // 初始化事件
ez_err_t ezos_event_get(ez_event_t* event, uint32_t and_mask, uint32_t or_mask, eztm_t timeout);  // 获取事件值
ez_err_t ezos_event_set(ez_event_t* event, uint32_t bits);                                        // 设置事件
ez_err_t ezos_event_set_irq(ez_event_t* event, uint32_t bits);                                    // 中断用设置事件
ez_err_t ezos_event_clear(ez_event_t* event, uint32_t bits);                                      // 清除事件
ez_err_t ezos_event_clear_irq(ez_event_t* event, uint32_t bits);                                  // 中断用清除事件
#endif

#ifdef EZOS_MQ
ez_err_t ezos_mq_init(ez_mq_t* mq, uint16_t msg_size, void* pool, uint16_t pool_size);  // 添加消息队列
ez_err_t ezos_mq_send(ez_mq_t* mq, void* msg);                                          // 发送消息
ez_err_t ezos_mq_send_irq(ez_mq_t* mq, void* msg);                                      // 中断用发送消息
ez_err_t ezos_mq_receive(ez_mq_t* mq, void* msg, eztm_t timeout);                       // 接收消息
ez_err_t ezos_mq_clear(ez_mq_t* mq);                                                    // 清空消息
#endif

#ifdef EZOS_MEM
typedef struct {
    task_name_t task[EZOS_MEM_BLK_NUM];  // 内存表位置索引任务名称
    uint8_t tbl[EZOS_MEM_BLK_NUM];       // 内存表, 标记连续内存块数量
    uint8_t pool[EZOS_MEM_POOL_SIZE];    // 内存池
} ezmm_map_t;

void* ezos_malloc(task_name_t task, uint16_t size, uint16_t* size_got);  // 申请或读取动态内存, 并返回当前指针.
void ezos_free(task_name_t name);                                        // 释放动态内存, 一般不需要调用. 任务删除时会自动释放
void ezos_mem_sort(void);                                                // 内存整理函数, 若配置为自动整理, 一般不需要调用.
void ezos_mem_clear(void);                                               // 强制清空所有动态内存, 一般不需要调用.

// ezos_malloc 和 ezos_free 简化函数
__STATIC_FORCEINLINE void* self_malloc(uint16_t size) {  // 当前任务, 申请或读取动态内存
    return ezos_malloc(ezos_self_name(), size, NULL);
}
__STATIC_FORCEINLINE void* self_mem_get(void) {  // 当前任务, 读取动态内存
    return ezos_malloc(ezos_self_name(), 0, NULL);
}
__STATIC_FORCEINLINE void* ezos_mem_get(task_name_t task) {  // 读取指定任务的动态内存.
    return ezos_malloc(task, 0, NULL);                       // 这样, 在指定任务生命周期内, 该动态内存就能全局使用.
}
__STATIC_FORCEINLINE void self_free(void) {  // 当前任务, 释放动态内存, 一般不需要调用. 任务删除时会自动释放
    ezos_free(ezos_self_name());
}

#ifdef EZOS_MEM_SHOW
void ezos_mem_show(void);  // 动态内存显示函数, 仅用于调试观察使用
#endif

#endif /* EZOS_MEM */

#ifdef __cplusplus
}
#endif

#endif /* EZOS_H */
