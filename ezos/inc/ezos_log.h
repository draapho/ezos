/**
 * \file            ezos_log.h
 * \brief           ezos log header file
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

#ifndef EZOS_LOG_H__
#define EZOS_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {  // LOG level
    EZOS_LOG_ERROR = 0,
    EZOS_LOG_DEBUG,
    EZOS_LOG_WARNING,
    EZOS_LOG_INFO,
};

/* 定义调试打印函数 */
#define ez_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)

#ifdef EZOS_LOG
#ifndef EZOS_LOG_LEVEL
#define EZOS_LOG_LEVEL EZOS_LOG_INFO
#endif
#define ezlog_line(lvl, fmt, ...) ez_printf(fmt, ##__VA_ARGS__);
#else
#define ezlog_line(lvl, fmt, ...)
#endif /* EZOS_LOG */

#define LOG(fmt, ...) ezlog_line("L", fmt, ##__VA_ARGS__)

#if (EZOS_LOG_LEVEL >= EZOS_LOG_INFO)
#define LOG_I(fmt, ...) ezlog_line("I", fmt, ##__VA_ARGS__)
#else
#define LOG_I(...)
#endif

#if (EZOS_LOG_LEVEL >= EZOS_LOG_WARNING)
#define LOG_W(fmt, ...) ezlog_line("W", fmt, ##__VA_ARGS__)
#else
#define LOG_W(...)
#endif

#if (EZOS_LOG_LEVEL >= EZOS_LOG_DEBUG)
#define LOG_D(fmt, ...) ezlog_line("D", fmt, ##__VA_ARGS__)
#else
#define LOG_D(...)
#endif

#if (EZOS_LOG_LEVEL >= EZOS_LOG_ERROR)
#define LOG_E(fmt, ...) ezlog_line("E", fmt, ##__VA_ARGS__)
#else
#define LOG_E(...)
#endif

/* ASSERT 宏定义 */
#ifdef EZOS_ASSERT
#if EZOS_ASSERT_FUN
ezos_assert(fmt, ...) ez_printf("[Oops] " fmt, ##__VA_ARGS__)
#define ASSERT(expr) ((expr) ? (void)0 : ezos_assert("%s at %s, L%d\n", #expr, __FUNCTION__, __LINE__)
#else
#define ASSERT(expr)                  \
    do {                              \
        volatile uint8_t goahead = 0; \
        while (!(expr)) {             \
            if (goahead) break;       \
        }                             \
    } while (0);
#endif /* EZOS_ASSERT_FUN */
#else
#define ASSERT(expr) ((void)0)
#endif /* EZOS_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* EZOS_LOG_H__ */
