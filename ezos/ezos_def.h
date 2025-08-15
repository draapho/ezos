/**
 * \file            ezos_def.c
 * \brief           ezos define and typedef header file
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

#ifndef EZOS_DEF_H__
#define EZOS_DEF_H__

#include <stddef.h>
#include <stdint.h>

#include "cmsis_compiler.h"
#include "ezos_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置无操作和中断开关函数 */
#define ezos_nop() __NOP()
#define ezos_disable_irq() __disable_irq()
#define ezos_enable_irq() __enable_irq()

/* 中断标记压栈模式
#define ezos_disable_irq()                      \
    {                                           \
        uint32_t ez_irq_mask = __get_PRIMASK(); \
        __disable_irq();
#define ezos_enable_irq()       \
    __set_PRIMASK(ez_irq_mask); \
    }
*/

/* 阻塞式延时函数 */
void delay_us(uint32_t us);

/* check ezos configure value */
#if (EZOS_TICK_MS <= 0)
#error EZOS_TICK_MS error!
#endif

#if (EZOS_TASK_MAX <= 0)
#error EZOS_TASK_MAX error!
#endif

#if (EZOS_MEM_BLK_SIZE < 4 || EZOS_MEM_BLK_SIZE > 512)
#error EZOS_MEM_BLK_SIZE error!
#endif

#define EZOS_MEM_BLK_MAX UINT8_MAX
#if (EZOS_MEM_BLK_NUM <= 0 || EZOS_MEM_BLK_NUM >= EZOS_MEM_BLK_MAX)
#error EZOS_MEM_BLK_NUM error!
#endif

#if (EZOS_MEM_SORT < 0 || EZOS_MEM_SORT > 3)
#error EZOS_MEM_SORT error!
#endif

#ifndef EZOS_LOG
#undef EZOS_MEM_SHOW
#endif

#ifdef EZOS_MEM_SHOW
#define EZOS_MEM_CLEAN
#else
#undef EZOS_MEM_CLEAN
#endif

#if (EZOS_ASSERT_FUN < 0 || EZOS_ASSERT_FUN > 1)
#error EZOS_ASSERT_FUN error!
#endif

#if (EZOS_LOG_LEVEL < 0 || EZOS_LOG_LEVEL > 3)
#error EZOS_LOG_LEVEL error!
#endif

#if (defined(EZOS_SEM) || defined(EZOS_MUTEX) || \
     defined(EZOS_EVENT) || defined(EZOS_MQ))
#define EZOS_IPC
#endif

/* ezos typedef */
typedef enum {            // ezos 函数返回值
    EZOS_OK = 0,          // 成功.
    EZOS_FAIL,            // 失败.
    EZOS_EXIST,           // 相关内容已存在.
    EZOS_ERR_MSK = 0x80,  // 错误掩码, 可通过 (err & EZOS_ERR_MSK) 判断是否错误.
    EZOS_ERROR,           // 错误.
    EZOS_ERR_OVER,        // 错误, 空间溢出.
} ez_err_t;

typedef enum {          // ezos 任务状态
    EZOS_RUNNING = 1,   // 任务运行状态
    EZOS_READY = 0,     // 任务就绪状态
    EZOS_TAKENAP = -1,  // 任务小憩状态, 等待轮询所有任务
    EZOS_FROZEN = -2,   // 任务冻结状态, 永久挂起
    EZOS_SUSPEND = -3,  // 任务挂起状态, 等待倒计时
    EZOS_DELETED = -4,  // 任务删除状态 或 任务不存在
} ez_status_t;

typedef int32_t eztm_t;  // 延时时间类型, 单位ms, 必须是有符号类型.

enum {                                   // ezos_delay 特殊值. EZos TiMe
    EZTM_NULL = (eztm_t)EZOS_READY,      // 不延时, 等待轮询高优先级任务
    EZTM_AWHILE = (eztm_t)EZOS_TAKENAP,  // 延时一会, 等待轮询所有任务
    EZTM_FOREVER = (eztm_t)EZOS_FROZEN,  // 永久挂起, 任务冻结状态
};

enum {                         // 任务通用状态机, EZos State Machine
    EZSM_INIT = (ezsm_t)0,     // 任务初始化处理
    EZSM_DONE = (ezsm_t)-1,    // 通知任务完成, 系统将自动删除任务
    EZSM_ERROR = (ezsm_t)-2,   // 任务错误处理
    EZSM_DEINIT = (ezsm_t)-3,  // 任务反初始化处理
};

#ifdef EZOS_IPC
typedef enum {          // IPC通讯类型
    EZIPC_INVALID = 0,  // 非法类型
    EZIPC_SEM,          // 信号量类型
    EZIPC_MUTEX,        // 互斥量类型
    EZIPC_EVENT,        // 事件类型
    EZIPC_MQ,           // 消息队列类型
} ez_ipc_t;

typedef struct {    // 信号量类型定义
    uint8_t value;  // 信号量值
    ez_ipc_t type;  // IPC类型
} ez_sem_t;

typedef struct {    // 互斥量类型定义
    uint8_t value;  // 互斥量值
    ez_ipc_t type;  // IPC类型
} ez_mutex_t;

typedef struct {     // 事件类型定义
    uint32_t value;  // 事件值
    ez_ipc_t type;   // IPC类型
} ez_event_t;

typedef struct {         // 消息队列类型定义
    void* pool;          // 缓存池指针
    uint16_t pool_size;  // 缓存池大小
    uint16_t msg_size;   // 消息长度
    uint16_t in;         // 消息队列发送偏移地址
    uint16_t out;        // 消息队列接收偏移地址
    uint8_t empty;       // 缓存池是否为空
    ez_ipc_t type;       // IPC类型
} ez_mq_t;
#endif /* EZOS_IPC */

/* Compiler Related Definitions */
#if defined(__CMSIS_COMPILER_H) /* cmsis has defined most of things */
#if defined(__ARMCC_VERSION)
#if (__ARMCC_VERSION < 6010050) /* armcc */
#define __SECTION(x) __attribute__((section(x)))
#else /* arm clang */
#define __SECTION(x) __attribute__((section(x)))
#endif                  /* __ARMCC_VERSION */
#elif defined(__GNUC__) /* GCC GNU */
#define __SECTION(x) __attribute__((section(x)))
#elif defined(__ICCARM__) /* IAR */
#define __SECTION(x) @x
#elif defined(__TI_ARM__) /* TI arm */
#define __SECTION(x) __attribute__((section(x)))
#else
#error Unknown compiler.
#endif /* __ARMCC_VERSION */

#elif defined(__GNUC__) /* GNU GCC Compiler */
#define __ASM __asm
#define __INLINE __inline
#define __STATIC_INLINE static __inline
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static __inline
#define __NO_RETURN __attribute__((__noreturn__))
#define __USED __attribute__((used))
#define __WEAK __attribute__((weak))
#define __PACKED __attribute__((packed, aligned(1)))
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#define __PACKED_UNION union __attribute__((packed, aligned(1)))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __SECTION(x) __attribute__((section(x)))
#define __RESTRICT __restrict

#elif defined(__ARMCC_VERSION) /* ARM Compiler */
#define __ASM __asm
#define __INLINE __inline
#define __STATIC_INLINE static __inline
#define __USED __attribute__((used))
#define __WEAK __attribute__((weak))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __SECTION(x) __attribute__((section(x)))
#define __RESTRICT __restrict

#if (__ARMCC_VERSION < 6010050) /* armcc */
#define __STATIC_FORCEINLINE static __forceinline
#define __NO_RETURN __declspec(noreturn)
#define __PACKED __attribute__((packed))
#define __PACKED_STRUCT __packed struct
#define __PACKED_UNION __packed union
#else /* arm clang */
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static __inline
#define __NO_RETURN __attribute__((__noreturn__))
#define __PACKED __attribute__((packed, aligned(1)))
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#define __PACKED_UNION union __attribute__((packed, aligned(1)))
#endif /* __ARMCC_VERSION */

#elif defined(__IAR_SYSTEMS_ICC__) /* IAR Compiler */
#define __ASM __asm
#define __INLINE inline
#define __STATIC_INLINE static inline
#define __FORCEINLINE _Pragma("inline=forced")
#define __STATIC_FORCEINLINE __FORCEINLINE __STATIC_INLINE
#define __RESTRICT __restrict

#if __ICCARM_V8
#define __NO_RETURN __attribute__((__noreturn__))
#define __USED __attribute__((used))
#define __WEAK __attribute__((weak))
#define __PACKED __attribute__((packed, aligned(1)))
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#define __PACKED_UNION union __attribute__((packed, aligned(1)))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __SECTION(x) __attribute__((section(x)))
#else
#define __NO_RETURN _Pragma("object_attribute=__noreturn")
#define __USED _Pragma("__root")
#define __WEAK _Pragma("__weak")
#define __PACKED __packed
#define __PACKED_STRUCT __packed struct
#define __PACKED_UNION __packed union
#define __ALIGNED(n) _Pragma(data_alignment = n)
#define __SECTION(x) @x
#endif

#elif defined(__TI_COMPILER_VERSION__) /* TI Compiler */
#define __ASM __asm
#define __INLINE inline
#define __STATIC_INLINE static inline
#define __STATIC_FORCEINLINE __STATIC_INLINE
#define __NO_RETURN __attribute__((noreturn))
#define __USED __attribute__((used))
#define __WEAK __attribute__((weak))
#define __PACKED __attribute__((packed))
#define __PACKED_STRUCT struct __attribute__((packed))
#define __PACKED_UNION union __attribute__((packed))
#define __ALIGNED(x) __attribute__((aligned(x)))
#define __SECTION(x) __attribute__((section(x)))
#define __RESTRICT
#warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.

#elif defined(_MSC_VER) /* Microsoft Compiler */
#define __ASM
#define __INLINE __inline
#define __STATIC_INLINE static __inline
#define __STATIC_FORCEINLINE __STATIC_INLINE
#define __NO_RETURN
#define __USED
#define __WEAK
#define __PACKED
#define __PACKED_STRUCT struct
#define __PACKED_UNION union
#define __ALIGNED(x) __declspec(align(n))
#define __SECTION(x)
#define __RESTRICT

#else
#error Unknown compiler.

#endif /* __CMSIS_COMPILER_H */

#ifdef __cplusplus
}
#endif

#endif /* EZOS_DEF_H__ */
