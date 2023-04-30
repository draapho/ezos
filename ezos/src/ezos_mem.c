/**
 * \file            ezos_mem.c
 * \brief           ezos memory source file
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
 * Date:            2023/04/30 - optimize ezos_mem_sort
 */

#include "ezos.h"

#ifdef EZOS_MEM

/* variable */
static ezmm_map_t mem;
extern ez_task_t idle;
extern ez_task_t *run;  // 指向正在运行的任务

/* function*/
void ezprv_mem_free(uint8_t i);
extern void ezprv_mem_set(uint8_t *dest, uint8_t val, uint16_t size);
extern void ezprv_mem_cpy(uint8_t *dest, uint8_t *src, uint16_t size);

/**
 * \brief           清空动态内存
 */
void ezos_mem_clear(void) {
    ezprv_mem_set(mem.task, EZOS_TASK_NAME_END | 0xFF, EZOS_MEM_BLK_NUM);  // | 0xFF, 便于观察调试
    ezprv_mem_set(mem.tbl, EZOS_MEM_BLK_NUM, EZOS_MEM_BLK_NUM);
#ifdef EZOS_MEM_CLEAN
    ezprv_mem_set(mem.pool, 0, EZOS_MEM_POOL_SIZE);
#endif
}

/**
 * \brief           申请或读取动态内存, 并返回当前指针.
 * \param[in]       task:       任务名称
 * \param[in]       size:       需要的内存大小. 对于已存在内存, 该值不得大于已分配内存!
 * \param[out]      size_got:   可以为NULL值. 否则用于存放分配到的内存大小.
 * \return          返回当前指针.
 *  \arg \c         NULL        参数错误 / 申请失败 / size 大于 已申请的内存大小
 */
void *ezos_malloc(task_name_t task, uint16_t size, uint16_t *size_got) {
#if (EZOS_MEM_SORT >= 2)
    uint8_t sorted = 0;
#endif
    uint8_t blk_need, free_num, i;
    ez_task_t *search;
    ASSERT(task < EZOS_TASK_NAME_END);
    if (task >= EZOS_TASK_NAME_END) return NULL;

    if (task == run->name) {
        i = run->mem;
    } else {  // 不是当前任务的动态内存, 需要找出指定任务
        for (search = idle.next; search->name < task;) {
            search = search->next;
        }
        if ((search->name == task) && (size == 0)) {
            i = search->mem;  // 取出该任务关联的内存表位置
        } else {
            return NULL;  // 仅允许在当前任务下申请动态内存
        }
    }

    if (i < EZOS_MEM_BLK_NUM) {                                                  // 该动态内存已存在
        ASSERT(mem.task[i] == task);                                             // mem.task 或 task->mem 有损坏
        if ((mem.task[i] != task) || (size > mem.tbl[i] * EZOS_MEM_BLK_SIZE)) {  // 尝试申请的size大于现有的size
            return NULL;                                                         // 返回 NULL
        } else {                                                                 // 否则
            if (size_got != NULL) {
                *size_got = mem.tbl[i] * EZOS_MEM_BLK_SIZE;
            }
            return (void *)&mem.pool[i * EZOS_MEM_BLK_SIZE];  // 返回指定的动态内存指针
        }
    }
    if ((size == 0) || (size > EZOS_MEM_POOL_SIZE)) {  // 确保后续计算不会溢出
        return NULL;
    }

    blk_need = size / EZOS_MEM_BLK_SIZE;
    if (size % EZOS_MEM_BLK_SIZE) ++blk_need;  // 计算出所需要的内存块数量

    for (;;) {
        for (i = 0, free_num = 0; i < EZOS_MEM_BLK_NUM; i += mem.tbl[i]) {
            if (mem.task[i] >= EZOS_TASK_NAME_END) {   // 找到空闲内存块
                if (mem.tbl[i] < blk_need) {           // 连续空闲内存块不够
                    free_num += mem.tbl[i];            // 统计当前空闲块总量
                } else {                               // 连续空闲内存块可用
                    free_num = mem.tbl[i] - blk_need;  // 记录剩余空闲块数量
                    ezprv_mem_set(&mem.task[i], run->name, blk_need);
                    ezprv_mem_set(&mem.tbl[i], blk_need, blk_need);
                    ezprv_mem_set(&mem.tbl[i + blk_need], free_num, free_num);
                    run->mem = i;  // 分配该内存块
                    if (size_got != NULL) {
                        *size_got = mem.tbl[i] * EZOS_MEM_BLK_SIZE;
                    }
                    return (void *)&mem.pool[i * EZOS_MEM_BLK_SIZE];  // 返回内存指针
                }
            }
        }

#if (EZOS_MEM_SORT >= 2)
        if (free_num < blk_need) {
            return NULL;  // 空间不足, 申请失败
        }
        if (!sorted) {
            ezos_mem_sort();  // 进行内存整理
            sorted = 1;
        } else {
            ASSERT(!sorted);  // 不应该运行到这里! 如果此处断言, 则内存表已损坏.
            return NULL;
        }
#else
        break;
#endif
    }

    return NULL;  // 申请失败
}

/**
 * \brief           释放动态内存
 * \param[in]       task:       任务名称
 */
void ezos_free(task_name_t task) {
    ez_task_t *search;
    uint8_t i;

    ASSERT(task < EZOS_TASK_NAME_END);
    if (task >= EZOS_TASK_NAME_END) return;

    if (task == run->name) {
        i = run->mem;
    } else {  // 不是当前任务的动态内存, 需要找出指定任务
        for (search = idle.next; search->name < task;) {
            search = search->next;
        }
        if (search->name == task) {
            i = search->mem;  // 取出该任务关联的内存表位置
            search->mem = EZOS_MEM_BLK_MAX;
        } else {
            return;  // 没有找到
        }
    }
    ezprv_mem_free(i);
}

/**
 * \brief           释放动态内存
 * \param[in]       i:       内存表位置
 */
void ezprv_mem_free(uint8_t i) {
    uint8_t start, end, size;

    if (i >= EZOS_MEM_BLK_NUM) return;  // 该动态内存已释放
#ifdef EZOS_MEM_CLEAN
    ezprv_mem_set(&mem.pool[i * EZOS_MEM_BLK_SIZE], 0, mem.tbl[i] * EZOS_MEM_BLK_SIZE);
#endif

    if (mem.task[i] < EZOS_TASK_NAME_END) {                                  // 内存块已申请
        ezprv_mem_set(&mem.task[i], EZOS_TASK_NAME_END | 0xFF, mem.tbl[i]);  // 设为空闲内存块
        start = i;                                                           // 默认起始值
        end = i + mem.tbl[i];                                                // 默认终点值
        if (i) {                                                             // 不是第一个内存块
            if (mem.task[i - 1] >= EZOS_TASK_NAME_END) {                     // 前一个是空闲内存块
                start = i - mem.tbl[i - 1];                                  // 获得空闲块起始偏移量
                ASSERT(start < i);                                           // mem.tbl 有损坏
                if (start > i) start = i;
            }
        }
        if (end < EZOS_MEM_BLK_NUM - 1) {               // 后续还有内存块存在
            if (mem.task[end] >= EZOS_TASK_NAME_END) {  // 后一个是空闲内存块
                end += mem.tbl[end];                    // 获得空闲块结束偏移量
                ASSERT(end <= EZOS_MEM_BLK_NUM);        // mem.tbl 有损坏
                if (end > EZOS_MEM_BLK_NUM) end = i + mem.tbl[i];
            }
        }
        size = end - start;                          // 新的空闲块数量
        ezprv_mem_set(&mem.tbl[start], size, size);  // 标记为空闲内存块
    }
    return;
}

/**
 * \brief           内存整理函数
 */
void ezos_mem_sort(void) {
#if (EZOS_MEM_SORT > 0)
    uint8_t i, tmpj, tmpn;
    int16_t j;
    ez_task_t *search;

#ifdef EZOS_MEM_SHOW
    LOG("before mem sort\n");
    ezos_mem_show();
#endif

    for (i = 0; i < EZOS_MEM_BLK_NUM;) {                                                           // 首次内存整理, 顺向寻找空闲内存块, 顺向寻找并填充匹配的已用内存块
        if (mem.task[i] >= EZOS_TASK_NAME_END) {                                                   // 找到空闲内存块
            for (j = i + mem.tbl[i], tmpj = 0, tmpn = 0; j < EZOS_MEM_BLK_NUM; j += mem.tbl[j]) {  // 顺向寻找已用内存块
                if (mem.task[j] < EZOS_TASK_NAME_END) {                                            // 找到已用内存块
                    if ((mem.tbl[j] <= mem.tbl[i]) && (mem.tbl[j] > tmpn)) {
                        tmpn = mem.tbl[j];
                        tmpj = j;                       // 尽量寻找最合适的已用内存块
                        if (tmpn == mem.tbl[i]) break;  // 找到大小一样的内存块,跳出j循环
                    }
                }
            }
            if (tmpj) {  // 需要移动内存
                ezprv_mem_cpy(&mem.pool[i * EZOS_MEM_BLK_SIZE], &mem.pool[tmpj * EZOS_MEM_BLK_SIZE], tmpn * EZOS_MEM_BLK_SIZE);
                ezprv_mem_set(&mem.task[i], mem.task[tmpj], tmpn);  // 标记为已用块
                j = mem.tbl[i] - tmpn;                              // 切分原空闲块, 记录切分后的大小
                ezprv_mem_set(&mem.tbl[i], tmpn, tmpn);             // 设置已用内存块大小
                for (search = idle.next; search->name < mem.task[tmpj];) {
                    search = search->next;
                }
                if (search->name == mem.task[tmpj]) search->mem = i;              // 重映射关联地址
                ezprv_mem_set(&mem.task[tmpj], EZOS_TASK_NAME_END | 0xFF, tmpn);  // 标记为空闲块
                ezprv_mem_set(&mem.tbl[tmpj], tmpn, tmpn);
                i += tmpn;                                             // 剩余的空闲块起始位置
                tmpn = j;                                              // 剩余的空闲块数量
                for (j += i; j < EZOS_MEM_BLK_NUM; j += mem.tbl[j]) {  // 寻找后续空闲块, 准备合并
                    if (mem.task[j] >= EZOS_TASK_NAME_END) {
                        tmpn += mem.tbl[j];
                    } else {
                        break;
                    }
                }
                ezprv_mem_set(&mem.tbl[i], tmpn, tmpn);  // 合并空闲块
                continue;                                // 对切分出的空闲块, 直接进入下一个i循环
            }
        }
        i += mem.tbl[i];  // 下一个内存块位置
    }

#ifdef EZOS_MEM_SHOW
    LOG("in mem sort\n");
    ezos_mem_show();
#endif

    for (i = 0; i < EZOS_MEM_BLK_NUM;) {                         // 二次内存整理, 顺向寻找空闲块, 顺向寻找已用块进行对齐
        if (mem.task[i] >= EZOS_TASK_NAME_END) {                 // 找到空闲内存块
            for (j = i + mem.tbl[i];; j += mem.tbl[j]) {         // 顺向寻找已用内存块
                if (j >= EZOS_MEM_BLK_NUM) {                     // 遍历完成, 不再有离散的已用内存块
                    tmpn = EZOS_MEM_BLK_NUM - i;                 // 整个空闲块的大小
                    if (mem.tbl[i] != tmpn) {                    // 需要更新内存表
                        ezprv_mem_set(&mem.tbl[i], tmpn, tmpn);  // 合并空闲块
                    }
#ifdef EZOS_MEM_CLEAN
                    ezprv_mem_set(&mem.pool[i * EZOS_MEM_BLK_SIZE], 0, tmpn * EZOS_MEM_BLK_SIZE);
#endif
                    i = EZOS_MEM_BLK_NUM;                       // 跳出i循环
                    break;                                      // 完成内存整理
                } else if (mem.task[j] < EZOS_TASK_NAME_END) {  // 找到已用内存块
                    tmpn = mem.tbl[i];                          // 空闲块大小
                    ezprv_mem_cpy(&mem.pool[i * EZOS_MEM_BLK_SIZE], &mem.pool[j * EZOS_MEM_BLK_SIZE], mem.tbl[j] * EZOS_MEM_BLK_SIZE);
                    ezprv_mem_set(&mem.task[i], mem.task[j], mem.tbl[j]);  // 更新使用标记
                    ezprv_mem_set(&mem.tbl[i], mem.tbl[j], mem.tbl[j]);    // 覆盖掉空闲块信息
                    for (search = idle.next; search->name < mem.task[j];) {
                        search = search->next;
                    }
                    if (search->name == mem.task[j]) search->mem = i;              // 重映射关联地址
                    i += mem.tbl[j];                                               // 空闲块偏移量
                    ezprv_mem_set(&mem.task[i], EZOS_TASK_NAME_END | 0xFF, tmpn);  // 标记空闲块
                    ezprv_mem_set(&mem.tbl[i], tmpn, tmpn);                        // 覆盖掉已用块信息
                    break;                                                         // 跳出j循环, 继续i循环
                }
            }
        } else {
            i += mem.tbl[i];
        }
    }

#ifdef EZOS_MEM_SHOW
    LOG("after mem sort\n");
    ezos_mem_show();
#endif

#endif /* EZOS_MEM_SORT */
}

#ifdef EZOS_MEM_SHOW
/**
 * \brief           动态内存显示函数, 调试观察使用
 */
void ezos_mem_show(void) {
    char *data;

    data = (char *)mem.pool;
    LOG("idx, task, tbl: pool blk\n");
    for (uint8_t i = 0; i < EZOS_MEM_BLK_NUM; i++) {
        LOG(" %02x, %04x,  %02x: ", i, mem.task[i], mem.tbl[i]);
        for (unsigned int j = 0; j < EZOS_MEM_BLK_SIZE; j++) {
            LOG("%02x ", *data++);
        }
        LOG("\n");
    }
}
#endif /* EZOS_MEM_SHOW */

#endif /* EZOS_MEM */
