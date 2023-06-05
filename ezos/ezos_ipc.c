/**
 * \file            ezos_ipc.c
 * \brief           ezos ipc source file
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

#ifdef EZOS_IPC

/* variable */
extern ez_task_t idle;
extern ez_task_t *run;

/* declare function*/
static ez_err_t prv_ipc_fail_process(void *ipc, eztm_t timeout);
extern void ezprv_mem_cpy(uint8_t *dest, uint8_t *src, uint16_t size);

#ifdef EZOS_SEM
/**
 * \brief           初始化信号量
 * \param[in]       sem:        信号量指针
 * \param[in]       value:      信号量初始值
 * \return          初始化信号量是否成功
 *  \arg \c         EZOS_OK     初始化成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_sem_init(ez_sem_t *sem, uint8_t value) {
    ASSERT(sem != NULL);
    if (sem == NULL) return EZOS_ERROR;

    ezos_disable_irq();
    sem->type = EZIPC_SEM;
    sem->value = value;
    ezos_enable_irq();
    return EZOS_OK;
}

/**
 * \brief           获取信号量. 只能在任务中使用
 * \param[in]       sem:        信号量指针
 * \param[in]       timeout:    信号量超时时间. 特殊值如下.
 *  \arg \c         >0                  设置超时时间, 单位ms.
 *  \arg \c         0                   不等待, 直接返回 EZOS_OK 或 EZOS_ERR_OVER
 *  \arg \c         <0/EZTM_FOREVER     永久等待.
 * \return          获取信号量是否成功
 *  \arg \c         EZOS_OK             获取成功.
 *  \arg \c         EZOS_FAIL           接收失败, 任务自动延时等待
 *  \arg \c         EZOS_ERROR          报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER       报错, 超时
 */
ez_err_t ezos_sem_take(ez_sem_t *sem, eztm_t timeout) {
    ez_err_t result = EZOS_ERROR;
    ASSERT(sem != NULL);
    ASSERT(sem->type == EZIPC_SEM);
    if ((sem == NULL) || (sem->type != EZIPC_SEM)) return EZOS_ERROR;

    ezos_disable_irq();
    if (sem->value) {
        --sem->value;
        run->ipc = NULL;
        result = EZOS_OK;  // 成功获取信号量
    } else {               // 失败
        result = prv_ipc_fail_process(sem, timeout);
    }
    ezos_enable_irq();
    return result;
}

/**
 * \brief           中断中调用, 释放信号量.
 * \param[in]       sem:        信号量指针
 * \return          释放信号量是否成功
 *  \arg \c         EZOS_OK     释放成功
 *  \arg \c         EZOS_ERROR  报错, 信号量不存在
 */
ez_err_t ezos_sem_release_irq(ez_sem_t *sem) {
    ez_task_t *task;
    ASSERT(sem != NULL);
    ASSERT(sem->type == EZIPC_SEM);
    if ((sem == NULL) || (sem->type != EZIPC_SEM)) return EZOS_ERROR;

    for (task = idle.next; task->name != EZOS_TASK_IDLE; task = task->next) {
        if (task->ipc == sem) {         // 任务正在等待该信号量
            task->status = EZOS_READY;  // 进入就绪状态竞争信号量
        }
    }
    if (sem->value < UINT8_MAX) ++sem->value;  // 递增信号量
    return EZOS_OK;
}

/**
 * \brief           释放信号量.
 * \param[in]       sem:        信号量指针
 * \return          释放信号量是否成功
 *  \arg \c         EZOS_OK     释放成功
 *  \arg \c         EZOS_ERROR  报错, 信号量不存在
 */
ez_err_t ezos_sem_release(ez_sem_t *sem) {
    ez_err_t result;

    ezos_disable_irq();
    result = ezos_sem_release_irq(sem);
    ezos_enable_irq();
    return result;
}
#endif /* EZOS_SEM */

#ifdef EZOS_MUTEX
/**
 * \brief           初始化互斥量
 * \param[in]       mutex:      互斥量指针
 * \param[in]       value:      互斥量初始值
 * \return          初始化互斥量是否成功
 *  \arg \c         EZOS_OK     初始化成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_mutex_init(ez_mutex_t *mutex, uint8_t value) {
    if (mutex == NULL) return EZOS_ERROR;

    ezos_disable_irq();
    mutex->type = EZIPC_MUTEX;
    mutex->value = value;
    ezos_enable_irq();
    return EZOS_OK;
}

/**
 * \brief           获取互斥量. 只能在任务中使用
 * \param[in]       mutex:      互斥量指针
 * \param[in]       timeout:    互斥量等待时间. 特殊值如下.
 *  \arg \c         >0                  设置超时时间, 单位ms.
 *  \arg \c         0                   不等待, 直接返回 EZOS_OK 或 EZOS_ERR_OVER
 *  \arg \c         <0/EZTM_FOREVER     永久等待.
 * \return          获取互斥量是否成功
 *  \arg \c         EZOS_OK             获取成功.
 *  \arg \c         EZOS_FAIL           接收失败, 任务自动延时等待
 *  \arg \c         EZOS_ERROR          报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER       报错, 超时
 */
ez_err_t ezos_mutex_take(ez_mutex_t *mutex, eztm_t timeout) {
    ez_err_t result = EZOS_ERROR;
    ASSERT(mutex != NULL);
    ASSERT(mutex->type == EZIPC_MUTEX);
    if ((mutex == NULL) || (mutex->type != EZIPC_MUTEX)) return EZOS_ERROR;

    ezos_disable_irq();
    if (mutex->value) {
        mutex->value = 0;
        run->ipc = NULL;
        result = EZOS_OK;  // 成功获取互斥量
    } else {               // 失败
        result = prv_ipc_fail_process(mutex, timeout);
    }
    ezos_enable_irq();
    return result;
}

/**
 * \brief           中断中调用, 释放互斥量.
 * \param[in]       mutex:      互斥量指针
 * \return          释放互斥量是否成功
 *  \arg \c         EZOS_OK     释放成功
 *  \arg \c         EZOS_ERROR  报错, 互斥量不存在
 */
ez_err_t ezos_mutex_release_irq(ez_mutex_t *mutex) {
    ez_task_t *task;
    ASSERT(mutex != NULL);
    ASSERT(mutex->type == EZIPC_MUTEX);
    if ((mutex == NULL) || (mutex->type != EZIPC_MUTEX)) return EZOS_ERROR;

    for (task = idle.next; task->name != EZOS_TASK_IDLE; task = task->next) {
        if (task->ipc == mutex) {       // 任务正在等待该互斥量
            task->status = EZOS_READY;  // 进入就绪状态竞争互斥量
        }
    }
    mutex->value = 1;  // 释放互斥量
    return EZOS_OK;
}

/**
 * \brief           释放互斥量.
 * \param[in]       mutex:      互斥量指针
 * \return          释放互斥量是否成功
 *  \arg \c         EZOS_OK     释放成功
 *  \arg \c         EZOS_ERROR  报错, 互斥量不存在
 */
ez_err_t ezos_mutex_release(ez_mutex_t *mutex) {
    ez_err_t result;

    ezos_disable_irq();
    result = ezos_mutex_release_irq(mutex);
    ezos_enable_irq();
    return result;
}
#endif /* EZOS_MUTEX */

#ifdef EZOS_EVENT
/**
 * \brief           初始化事件
 * \param[in]       event:      事件指针
 * \param[in]       value:      事件初始值
 * \return          初始化事件是否成功
 *  \arg \c         EZOS_OK     初始化成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_event_init(ez_event_t *event, uint32_t value) {
    ASSERT(event != NULL);
    if (event == NULL) return EZOS_ERROR;

    ezos_disable_irq();
    event->type = EZIPC_EVENT;
    event->value = value;
    ezos_enable_irq();
    return EZOS_OK;
}

/**
 * \brief           获取事件值. 事件值不会自动更改, 需要手动清除.
 * \param[in]       event:      事件指针
 * \param[in]       and_mask:   需要的与事件掩码, 置0忽略与事件.
 * \param[in]       or_mask:    需要的或事件掩码, 置0忽略或事件.
 * \param[in]       timeout:    互斥量等待时间. 特殊值如下.
 *  \arg \c         >0                  设置超时时间, 单位ms.
 *  \arg \c         0                   不等待, 直接返回 EZOS_OK 或 EZOS_ERR_OVER
 *  \arg \c         <0/EZTM_FOREVER     永久等待.
 * \return          获取事件是否成功
 *  \arg \c         EZOS_OK     获取成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_event_get(ez_event_t *event, uint32_t and_mask, uint32_t or_mask, eztm_t timeout) {
    ez_err_t result = EZOS_ERROR;
    uint32_t value;
    ASSERT(event != NULL);
    ASSERT(event->type == EZIPC_EVENT);
    if ((event == NULL) || (event->type != EZIPC_EVENT)) return EZOS_ERROR;

    ezos_disable_irq();
    value = event->value;
    if ((and_mask && ((value & and_mask) == and_mask)) || (value & or_mask)) {
        run->ipc = NULL;
        result = EZOS_OK;  // 成功获取事件
    } else {
        result = prv_ipc_fail_process(event, timeout);
    }
    ezos_enable_irq();
    return result;
}

/**
 * \brief           中断中调用, 设置事件.
 * \param[in]       event:      事件指针
 * \param[in]       bits:       相关位置一
 * \return          设置事件是否成功
 *  \arg \c         EZOS_OK     设置成功
 *  \arg \c         EZOS_ERROR  报错, 事件不存在
 */
ez_err_t ezos_event_set_irq(ez_event_t *event, uint32_t bits) {
    ASSERT(event != NULL);
    ASSERT(event->type == EZIPC_EVENT);
    if ((event == NULL) || (event->type != EZIPC_EVENT)) return EZOS_ERROR;

    event->value |= bits;
    return EZOS_OK;
}

/**
 * \brief           设置事件.
 * \param[in]       event:      事件指针
 * \param[in]       bits:       相关位置一
 * \return          设置事件是否成功
 *  \arg \c         EZOS_OK     设置成功
 *  \arg \c         EZOS_ERROR  报错, 事件不存在
 */
ez_err_t ezos_event_set(ez_event_t *event, uint32_t bits) {
    ez_err_t result;

    ezos_disable_irq();
    result = ezos_event_set_irq(event, bits);
    ezos_enable_irq();
    return result;
}

/**
 * \brief           中断中调用, 清除事件.
 * \param[in]       event:      事件指针
 * \param[in]       bits:       相关位置一
 * \return          清除事件是否成功
 *  \arg \c         EZOS_OK     清除成功
 *  \arg \c         EZOS_ERROR  报错, 事件不存在
 */
ez_err_t ezos_event_clear_irq(ez_event_t *event, uint32_t bits) {
    ASSERT(event != NULL);
    ASSERT(event->type == EZIPC_EVENT);
    if ((event == NULL) || (event->type != EZIPC_EVENT)) return EZOS_ERROR;

    event->value &= ~bits;
    return EZOS_OK;
}

/**
 * \brief           清除事件.
 * \param[in]       event:      事件指针
 * \param[in]       bits:       相关位清零
 * \return          清除事件是否成功
 *  \arg \c         EZOS_OK     清除成功
 *  \arg \c         EZOS_ERROR  报错, 事件不存在
 */
ez_err_t ezos_event_clear(ez_event_t *event, uint32_t bits) {
    ez_err_t result;

    ezos_disable_irq();
    result = ezos_event_clear_irq(event, bits);
    ezos_enable_irq();
    return result;
}
#endif /* EZOS_EVENT */

#ifdef EZOS_MQ
/**
 * \brief           初始化消息队列
 * \param[in]       mq:         消息队列指针
 * \param[in]       msg_size:   消息长度
 * \param[in]       pool:       消息队列缓存池指针
 * \param[in]       pool_size:  消息队列缓存池大小
 * \return          初始化消息队列是否成功
 *  \arg \c         EZOS_OK     初始化成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_mq_init(ez_mq_t *mq, uint16_t msg_size, void *pool, uint16_t pool_size) {
    ASSERT(mq != NULL);
    ASSERT(pool != NULL);
    ASSERT(msg_size > 0);
    ASSERT(pool_size >= msg_size);
    if ((mq == NULL) || (pool == NULL) || (msg_size == 0) || (pool_size < msg_size)) return EZOS_ERROR;

    ezos_disable_irq();
    mq->type = EZIPC_MQ;
    mq->pool = pool;
    mq->msg_size = msg_size;
    mq->pool_size = msg_size * (pool_size / msg_size);  // 取整
    mq->in = 0;
    mq->out = 0;
    mq->empty = 1;
    ezos_enable_irq();
    return EZOS_OK;
}

/**
 * \brief           中断中调用, 发送消息, 大小为 msg_size
 * \param[in]       mq:         消息队列指针
 * \param[in]       msg:        需发送的消息指针
 * \return          发送消息队列是否成功
 *  \arg \c         EZOS_OK         发送成功
 *  \arg \c         EZOS_ERROR      报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER   报错, 缓存池溢出
 */
ez_err_t ezos_mq_send_irq(ez_mq_t *mq, void *msg) {
    ez_task_t *task;
    ASSERT(mq != NULL);
    // ASSERT(msg != NULL); // 允许输入空值
    ASSERT(mq->type == EZIPC_MQ);
    if ((mq == NULL) || (msg == NULL) || (mq->type != EZIPC_MQ)) return EZOS_ERROR;
    if ((mq->in == mq->out) && (!mq->empty)) return EZOS_ERR_OVER;  // 缓存池溢出

    for (task = idle.next; task->name != EZOS_TASK_IDLE; task = task->next) {
        if (task->ipc == mq) {          // 任务正在等待该消息队列
            task->status = EZOS_READY;  // 进入就绪状态竞争消息队列
        }
    }
    ezprv_mem_cpy((uint8_t *)(mq->pool) + mq->in, msg, mq->msg_size);  // 将消息加入到消息队列中
    mq->empty = 0;
    mq->in += mq->msg_size;
    if (mq->in >= mq->pool_size) mq->in = 0;
    return EZOS_OK;
}

/**
 * \brief           发送消息, 大小为 msg_size
 * \param[in]       mq:         消息队列指针
 * \param[in]       msg:        需发送的消息指针
 * \return          发送消息队列是否成功
 *  \arg \c         EZOS_OK         发送成功
 *  \arg \c         EZOS_ERROR      报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER   报错, 缓存池溢出
 */
ez_err_t ezos_mq_send(ez_mq_t *mq, void *msg) {
    ez_err_t result;

    ezos_disable_irq();
    result = ezos_mq_send_irq(mq, msg);
    ezos_enable_irq();
    return result;
}

/**
 * \brief           接收消息, 大小为 msg_size
 * \param[in]       mq:         消息队列指针
 * \param[in]       msg:        接收消息的指针
 * \param[in]       timeout:    消息队列等待时间. 特殊值如下.
 *  \arg \c         >0                  设置超时时间, 单位ms.
 *  \arg \c         0                   不等待, 直接返回 EZOS_OK 或 EZOS_ERR_OVER
 *  \arg \c         <0/EZTM_FOREVER     永久等待.
 * \return          接收消息队列是否成功
 *  \arg \c         EZOS_OK             接收成功, 状态机自动递增
 *  \arg \c         EZOS_FAIL           接收失败, 任务自动延时等待
 *  \arg \c         EZOS_ERROR          报错, 非法参数
 *  \arg \c         EZOS_ERR_OVER       报错, 超时
 */
ez_err_t ezos_mq_receive(ez_mq_t *mq, void *msg, eztm_t timeout) {
    ez_err_t result = EZOS_ERROR;
    ASSERT(mq != NULL);
    ASSERT(msg != NULL);  // 接收端, msg必须未有效指针
    ASSERT(mq->type == EZIPC_MQ);
    if ((mq == NULL) || (msg == NULL) || (mq->type != EZIPC_MQ)) return EZOS_ERROR;

    ezos_disable_irq();
    if (!mq->empty) {
        ezprv_mem_cpy(msg, (uint8_t *)(mq->pool) + mq->out, mq->msg_size);
        mq->out += mq->msg_size;
        if (mq->out >= mq->pool_size) mq->out = 0;
        if (mq->in == mq->out) mq->empty = 1;
        run->ipc = NULL;
        result = EZOS_OK;  // 从消息队列中取出消息
    } else {               // 失败
        result = prv_ipc_fail_process(mq, timeout);
    }
    ezos_enable_irq();
    return result;
}

/**
 * \brief           清空消息队列
 * \param[in]       mq:         消息队列指针
 * \return          清空消息队列是否成功
 *  \arg \c         EZOS_OK     清空成功
 *  \arg \c         EZOS_ERROR  报错, 非法参数
 */
ez_err_t ezos_mq_clear(ez_mq_t *mq) {
    ASSERT(mq != NULL);
    ASSERT(mq->type == EZIPC_MQ);
    if ((mq == NULL) || (mq->type != EZIPC_MQ)) return EZOS_ERROR;

    ezos_disable_irq();
    mq->in = 0;
    mq->out = 0;
    mq->empty = 1;
    ezos_enable_irq();
    return EZOS_OK;
}
#endif /* EZOS_MQ */

/**
 * \brief           私有函数, 尝试获取IPC失败后的相关处理
 */
static ez_err_t prv_ipc_fail_process(void *ipc, eztm_t time_ms) {
    if (run->ipc != ipc) {  // 首次失败, 设置run->delay
        if (time_ms < 0) time_ms = EZTM_FOREVER;
#if (EZOS_TICK_MS > 1)
        else if (time_ms > 0) {  // 将ms的延时值转换为基础时钟的延时值
            if (time_ms % EZOS_TICK_MS) time_ms += EZOS_TICK_MS;
            time_ms /= EZOS_TICK_MS;
        }
#endif
        run->delay = time_ms;
    }

    if (run->delay == 0) {  // 首次或多次获取IPC失败, 已超时
        run->ipc = NULL;
        return EZOS_ERR_OVER;  // 超时
    } else {                   // 获取IPC失败, 未超时, 重新设置任务状态
        run->ipc = ipc;
        if (run->delay > 0) {
            run->status = EZOS_SUSPEND;
        } else {
            run->status = EZOS_FROZEN;
        }
        return EZOS_FAIL;
    }
}

#endif /* EZOS_IPC */
