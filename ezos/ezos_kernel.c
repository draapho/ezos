/**
 * \file            ezos_kernel.c
 * \brief           ezos kernel source file
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
#include "ezos.h"

/* variable */
static ez_task_t tasks[EZOS_TASK_MAX];  // ezos任务链表空间
static ez_task_t *trash;                // 回收链表起始节点
ez_task_t *run;                         // 正在运行的任务指针
ez_task_t idle;                         // 系统空闲任务, 任务链表起始节点
void (*idle_hook)(void);                // 空闲任务钩子函数

#if defined EZOS_TEST
static ez_test_tick_t test;    // 性能测试基准时间
static ez_test_t performance;  // 用于性能测试
#endif

/* declare function*/
static ezsm_t ezos_task_idle(ezsm_t, void *para);
static ez_err_t ezprv_set_task(task_name_t name, eztm_t time_ms);
void ezprv_mem_set(uint8_t *dest, uint8_t val, uint16_t size);
void ezprv_mem_cpy(uint8_t *dest, uint8_t *src, uint16_t size);

#ifdef EZOS_MEM
extern void ezprv_mem_free(uint8_t i);
#endif /* EZOS_MEM */

// 任务函数数组
static ezsm_t (*const task_pfun[EZOS_TASK_NAME_END])(ezsm_t, void *) = {
#define X(name, task, ...) task,  // 与任务名称的enum值相对应
    EZOS_TASKS_NAME_FUN
#undef X
        ezos_task_idle,  // 对应 EZOS_TASK_IDLE
};

/**
 * \brief           ezos初始化函数
 */
void ezos_init(void) {
    uint8_t i;

    idle.next = &idle;  // 任务链表为循环链表
    idle.name = EZOS_TASK_IDLE;
    idle.status = EZOS_READY;
    idle.state = EZSM_INIT;
    idle.para = NULL;
    idle_hook = NULL;  // 空闲任务钩子函数
#ifdef EZOS_IPC
    idle.ipc = NULL;
#endif
#ifdef EZOS_MEM
    idle.mem = EZOS_MEM_BLK_MAX;
#endif

    trash = &tasks[0];                           // 回收链表起始节点
    for (i = 0; i < (EZOS_TASK_MAX - 1); i++) {  // 回收链表为单向链表
        tasks[i].next = &tasks[i + 1];
        tasks[i].name = EZOS_TASK_IDLE;
        tasks[i].status = EZOS_DELETED;
    }
    tasks[EZOS_TASK_MAX - 1].next = NULL;  // 回收链表结束节点
    run = &idle;

#ifdef EZOS_TEST
    test.idle_gap_tick = 0;
    test.task_run_tick = 0;
    performance.idle_gap_cur = 0;
    performance.idle_gap_max = 0;
    performance.task_run_cur = 0;
    performance.task_run_max = 0;
    performance.task_run_state = EZSM_INIT;
    performance.task_run_name = EZOS_TASK_IDLE;
    performance.trash_num = EZOS_TASK_MAX;
    performance.trash_num_min = EZOS_TASK_MAX;
#endif

#ifdef EZOS_MEM
    ezos_mem_clear();
#endif
}

/**
 * \brief           ezos 系统时钟定时扫描. 在定时中断中调用.
 */
void ezos_tick_irq(void) {
    ez_task_t *task;
    ASSERT(run != NULL);  // 必须先调用 ezos_init();

    for (task = idle.next; task->name != EZOS_TASK_IDLE; task = task->next) {  // 遍历任务链表
        if (task->delay > 0) {                                                 // 任务等待时间倒计时
            if (--task->delay == 0) {
                task->status = EZOS_READY;
            }
        }
    }
}

/**
 * \brief           任务调度函数, 放在main函数的最后.
 */
void ezos_schedule(void) {
    ez_task_t *task;
    ezsm_t state;
    ASSERT(run != NULL);  // 必须先调用 ezos_init();

    for (;;) {
        ezos_disable_irq();
#ifdef EZOS_TEST
        performance.task_run_cur = test.task_run_tick;
        test.task_run_tick = 0;
        if (performance.task_run_max < performance.task_run_cur) {
            performance.task_run_max = performance.task_run_cur;
            performance.task_run_name = run->name;
            performance.task_run_state = run->state;
        }
#endif
        if (run->status == EZOS_RUNNING) {  // 用户未设置任务延时
            run->delay = 0;
            run->status = EZOS_TAKENAP;  // 等待轮询所有任务
        }
        for (task = idle.next; task->status != EZOS_READY; task = task->next) {  // 寻找就绪的任务
            ;
        }
        run = task;
        run->status = EZOS_RUNNING;
        ezos_enable_irq();

        state = task_pfun[run->name](run->state, run->para);  // 运行任务函数

        if (state == EZSM_DONE) {    // 状态机任务退出
            ezos_delete(run->name);  // 自动删除
        } else {
            run->state = state;
        }
    }
}

/**
 * \brief           添加任务, 任务状态初始化为 EZSM_INIT, 0 值.
 * \param[in]       name:   任务名称, 同时也是任务优先级, 值越小优先级越高
 * \param[in]       para:   函数参数
 * \param[in]       time_ms:    延时时间, 单位 ms. 特殊值如下:
 *  \arg \c         EZTM_NULL       等待轮询高优先级任务
 *  \arg \c         EZTM_AWHILE     等待轮询所有任务
 *  \arg \c         EZTM_FOREVER    永久等待
 * \return          添加任务是否成功
 *  \arg \c         EZOS_OK         添加任务成功
 *  \arg \c         EZOS_EXIST      添加任务失败, 任务已存在
 *  \arg \c         EZOS_ERROR      报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER   报错, 任务已满
 */
ez_err_t ezos_add(task_name_t name, void *para, eztm_t time_ms) {
    ez_task_t *task, *pre, *search;
    ASSERT(name < EZOS_TASK_IDLE);
    if (name >= EZOS_TASK_IDLE) return EZOS_ERROR;  // 非法参数

    for (pre = &idle, search = idle.next; search->name < name;) {  // 寻找要加入的前后节点
        pre = search;
        search = search->next;
    }
    if (search->name == name) {  // 任务已存在
        return EZOS_EXIST;
    } else {                                      // 任务不在链表中, 从回收链表中取出一个节点
        if (trash == NULL) return EZOS_ERR_OVER;  // 任务链表已满, 回收链表空间不足
        task = trash;                             // 取出该回收链表节点
        trash = trash->next;
    }

    if (time_ms <= EZTM_FOREVER) time_ms = EZTM_FOREVER;
#if (EZOS_TICK_MS > 1)
    else if (time_ms > 0) {  // 将ms的延时值转换为基础时钟的延时值
        if (time_ms % EZOS_TICK_MS) time_ms += EZOS_TICK_MS;
        time_ms /= EZOS_TICK_MS;
    }
#endif

    ezos_disable_irq();
    task->next = search;
    pre->next = task;   // 加入任务节点
    task->name = name;  // 给任务节点赋值
#ifdef EZOS_IPC
    task->ipc = NULL;
#endif
    task->delay = time_ms;
    if (time_ms > 0) {
        task->status = EZOS_SUSPEND;
    } else {
        task->status = (ez_status_t)time_ms;
    }
    ezos_enable_irq();

    task->para = para;
    task->state = EZSM_INIT;
#ifdef EZOS_MEM
    task->mem = EZOS_MEM_BLK_MAX;
#endif
#ifdef EZOS_TEST
    if (--performance.trash_num < performance.trash_num_min) {
        performance.trash_num_min = performance.trash_num;
    }
#endif
    return EZOS_OK;  // 添加任务成功
}

/**
 * \brief           删除任务. 用户一般不使用, 而是让任务运行完后自动删除
 * \param[in]       name:   任务名称
 * \return          删除任务是否成功
 *  \arg \c         EZOS_OK     删除任务成功
 *  \arg \c         EZOS_FAIL   删除任务失败, 没有找到任务
 */
ez_err_t ezos_delete(task_name_t name) {
    ez_task_t *pre, *search;
    ASSERT(name < EZOS_TASK_IDLE);
    if (name >= EZOS_TASK_IDLE) return EZOS_FAIL;

    for (pre = &idle, search = idle.next; search->name < name;) {  // 寻找任务
        pre = search;
        search = search->next;
    }
    if (search->name == name) {  // 任务已找到
#ifdef EZOS_MEM                  // 删除任务时, 自动释放任务相关的动态内存
        uint8_t i = search->mem;
        search->mem = EZOS_MEM_BLK_MAX;
        ezprv_mem_free(i);
#endif /* EZOS_MEM */

        ezos_disable_irq();
#ifdef EZOS_IPC
        search->ipc = NULL;
#endif
        search->name = EZOS_TASK_IDLE;  // 删除任务
        search->status = EZOS_DELETED;  // 设为删除状态
        pre->next = search->next;       // 从任务链表中删除任务节点
        search->next = trash;           // 添加到删除链表
        ezos_enable_irq();
        trash = search;
#ifdef EZOS_TEST
        ++performance.trash_num;
#endif
        return EZOS_OK;  // 删除任务成功
    } else {
        return EZOS_FAIL;  // 没有找到任务
    }
}

/**
 * \brief           当前任务延时.
 * \param[in]       time_ms:    延时时间, 单位 ms. 特殊值如下:
 *  \arg \c         EZTM_NULL       等待轮询高优先级任务
 *  \arg \c         EZTM_AWHILE     等待轮询所有任务
 *  \arg \c         EZTM_FOREVER    永久等待
 * \return          延时任务是否成功
 *  \arg \c         EZOS_OK         当前任务延时成功
 *  \arg \c         EZOS_ERROR      报错, 非法参数
 */
ez_err_t ezos_delay(eztm_t time_ms) {
    ASSERT(run->name != EZOS_TASK_IDLE);

    if (time_ms <= EZTM_FOREVER) time_ms = EZTM_FOREVER;
#if (EZOS_TICK_MS > 1)
    else if (time_ms > 0) {  // 将ms的延时值转换为基础时钟的延时值
        if (time_ms % EZOS_TICK_MS) time_ms += EZOS_TICK_MS;
        time_ms /= EZOS_TICK_MS;
    }
#endif

    ezos_disable_irq();
    run->delay = time_ms;
    if (time_ms > 0) {
        run->status = EZOS_SUSPEND;
    } else {
        run->status = (ez_status_t)time_ms;
    }
    ezos_enable_irq();
    return EZOS_OK;
}

/**
 * \brief           重新设定指定任务的状态.
 * \param[in]       name:       任务名称, 同时也是任务优先级, 值越小优先级越高
 * \param[in]       time_ms:    延时时间, 单位 ms. 特殊值如下:
 *  \arg \c         EZTM_NULL       等待轮询高优先级任务
 *  \arg \c         EZTM_AWHILE     等待轮询所有任务
 *  \arg \c         EZTM_FOREVER    永久等待
 * \return          设置任务是否成功
 *  \arg \c         EZOS_OK         设置任务成功
 *  \arg \c         EZOS_FAIL       设置任务失败, 没有找到任务
 *  \arg \c         EZOS_ERROR      报错, 非法参数.
 */
ez_err_t ezprv_set_task(task_name_t name, eztm_t time_ms) {
    ez_task_t *search;
    ASSERT(name < EZOS_TASK_IDLE);
    if (name >= EZOS_TASK_IDLE) return EZOS_ERROR;

    // 内部使用的函数, 不需要检查 time_ms
    // if (time_ms <= EZTM_FOREVER) time_ms = EZTM_FOREVER;
    // #if (EZOS_TICK_MS > 1)
    //     else if (time_ms > 0) {
    //         if (time_ms % EZOS_TICK_MS) time_ms += EZOS_TICK_MS;
    //         time_ms /= EZOS_TICK_MS;
    //     }
    // #endif

    for (search = idle.next; search->name < name;) {  // 寻找任务
        search = search->next;
    }
    if (search->name == name) {  // 任务已找到
#ifdef EZOS_IPC
        search->ipc = NULL;  // 避免信号量意外激活任务
#endif
        search->delay = time_ms;
        if (time_ms > 0) {
            search->status = EZOS_SUSPEND;
        } else {
            search->status = (ez_status_t)time_ms;
        }
        return EZOS_OK;
    } else {
        return EZOS_FAIL;
    }
}

/**
 * \brief           中断中, 恢复指定任务. 不建议和IPC混合使用!!!
 * \param[in]       name:           任务名称
 * \return          设置任务是否成功
 *  \arg \c         EZOS_OK         设置任务成功
 *  \arg \c         EZOS_FAIL       设置任务失败, 没有找到任务
 *  \arg \c         EZOS_ERROR      报错, 非法参数. 不能是运行中的任务
 */
ez_err_t ezos_resume_irq(task_name_t name) {
    return ezprv_set_task(name, EZTM_NULL);
}

/**
 * \brief           恢复指定任务. 不建议和IPC混合使用!!!
 * \param[in]       name:           任务名称
 * \return          设置任务是否成功
 *  \arg \c         EZOS_OK         设置任务成功
 *  \arg \c         EZOS_FAIL       设置任务失败, 没有找到任务
 *  \arg \c         EZOS_ERROR      报错, 非法参数. 不能是运行中的任务
 */
ez_err_t ezos_resume(task_name_t name) {
    ez_err_t result;
    ezos_disable_irq();
    result = ezprv_set_task(name, EZTM_NULL);
    ezos_enable_irq();
    return result;
}

/**
 * \brief           冻结指定任务. 不建议和IPC混合使用!!!
 * \param[in]       name:           任务名称
 * \return          设置任务是否成功
 *  \arg \c         EZOS_OK         设置任务成功
 *  \arg \c         EZOS_FAIL       设置任务失败, 没有找到任务
 *  \arg \c         EZOS_ERROR      报错, 非法参数. 不能是运行中的任务
 */
ez_err_t ezos_frozen(task_name_t name) {
    ez_err_t result;
    ezos_disable_irq();
    result = ezprv_set_task(name, EZTM_FOREVER);
    ezos_enable_irq();
    return result;
}

/**
 * \brief           获取当前任务的任务名称.
 */
task_name_t ezos_self_name(void) {
    return run->name;
}

/**
 * \brief           获取当前任务的指针.
 */
const ez_task_t *ezos_self_ptr(void) {
    return run;
}

/**
 * \brief           获取任务状态.
 * \param[in]       name:   任务名称
 * \return          任务当前状态.
 *  \arg \c         EZOS_RUNNING    任务运行状态
 *  \arg \c         EZOS_READY      任务就绪状态, 等待运行
 *  \arg \c         EZOS_TAKENAP    任务小憩状态, 等待轮询所有任务
 *  \arg \c         EZOS_SUSPEND    任务挂起状态, 等待倒计时
 *  \arg \c         EZOS_FROZEN     任务冻结状态, 永久挂起
 *  \arg \c         EZOS_DELETED    任务删除状态 或 任务不存在
 */
ez_status_t ezos_status(task_name_t name) {
    ez_task_t *search;
    ASSERT(name < EZOS_TASK_IDLE);
    if (name >= EZOS_TASK_IDLE) return EZOS_DELETED;

    for (search = idle.next; search->name < name;) {  // 寻找任务
        search = search->next;
    }
    if (search->name == name) {  // 找到任务
        ez_status_t status;
        ezos_disable_irq();
        status = search->status;
        ezos_enable_irq();
        return status;
    } else {
        return EZOS_DELETED;
    }
}

/**
 * \brief           判断所有任务是否执行完成, 可用于辅助判断系统是否可以进入睡眠状态
 * \return          所有任务是否执行完成.
 *  \arg \c         EZOS_OK     所有任务完成, 系统可进入睡眠状态
 *  \arg \c         EZOS_FAIL   任务未全部完成.
 */
ez_err_t ezos_done(void) {
    ez_task_t *search;
    ez_err_t result = EZOS_OK;

    ezos_disable_irq();
    for (search = idle.next; search->name != EZOS_TASK_IDLE; search = search->next) {
        if (search->status > EZOS_FROZEN) {
            result = EZOS_FAIL;  // 依旧有任务需要运行
            break;
        }
    }
    ezos_enable_irq();
    return result;  // 没有需要执行的任务, 所有任务都已删除或处于永久冻结状态
}

#ifdef EZOS_TEST
/**
 * \brief           获取ezos性能测试数据
 * \return          返回性能测试数据
 */
void ezos_idle_gap_tick_increase(void) {
    test.idle_gap_tick++;
}

/**
 * \brief           获取ezos性能测试数据
 * \return          返回性能测试数据
 */
void ezos_task_run_tick_increase(void) {
    test.task_run_tick++;
}

/**
 * \brief           获取ezos性能测试数据
 * \return          返回性能测试数据
 */
ez_test_t ezos_get_test_performance(void) {
    return performance;
}
#endif /* EZOS_TEST */

/**
 * \brief           设置空闲任务钩子函数
 * \param[in]       hook: 钩子函数
 */
void ezos_set_idle_hook(void (*hook)(void)) {
    idle_hook = hook;
}

/**
 * \brief           ezos空闲任务函数
 */
static ezsm_t ezos_task_idle(ezsm_t state, void *para) {
    ez_task_t *search;

    ezos_disable_irq();                                          // 原子操作
    for (search = idle.next; search->name != EZOS_TASK_IDLE;) {  // 遍历任务链表
        if (search->status == EZOS_TAKENAP) {                    // 找到小憩状态的任务
            search->status = EZOS_READY;                         // 设置为就绪状态
        }
        search = search->next;
    }
#ifdef EZOS_TEST
    performance.idle_gap_cur = test.idle_gap_tick;
    test.idle_gap_tick = 0;
#endif
    ezos_enable_irq();

#ifdef EZOS_TEST
    do {
        static uint8_t not_first_call;  // 不记录首次调用之前的时间
        if (not_first_call) {
            if (performance.idle_gap_max < performance.idle_gap_cur) {
                performance.idle_gap_max = performance.idle_gap_cur;
            }
        } else {
            not_first_call = 1;
        }
    } while (0);
#endif /* EZOS_TEST */

#if defined(EZOS_MEM) && (EZOS_MEM_SORT == 3)
    ezos_mem_sort();
#endif

    if (idle_hook != NULL) {
        idle_hook();  // 调用空闲任务钩子函数
    }
    idle.status = EZOS_READY;
    return state;
}

void ezprv_mem_set(uint8_t *dest, uint8_t val, uint16_t size) {
    while (size--)
        *dest++ = val;
}

void ezprv_mem_cpy(uint8_t *dest, uint8_t *src, uint16_t size) {
    while (size--)
        *dest++ = *src++;
}
