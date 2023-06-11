/**
 * \file            ezos_port.c
 * \brief           ezos port source file
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

/* 重定向 printf */
#if 1  // 使用 syscalls.c

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#include "drv_uart.h"
PUTCHAR_PROTOTYPE {
    uart_tx(&UART_MCU, (uint8_t *)&ch, 1);
    return ch;
}

#else  // 使用更底层的标准库

#if defined(__GNUC__)
int _write(int fd, char *ptr, int len) {
    // HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
#elif defined(__ICCARM__)
#include "LowLevelIOInterface.h"
size_t __write(int handle, const unsigned char *buffer, size_t size) {
    // HAL_UART_Transmit(&huart1, (uint8_t *)buffer, size, HAL_MAX_DELAY);
    return size;
}
#elif defined(__CC_ARM)
int fputc(int ch, FILE *f) {
    // HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif /* __GNUC__ */

#endif /* 重定向 printf */

/* 调试打印内存信息 */
#include "ezos_log.h"
void log_dump(char *desc, uint8_t *buf, uint16_t size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if ((i % 8) == 0) {
            ez_printf("[%04x] ", (uint16_t)i);
        }
        ez_printf("%02x ", (uint16_t)buf[i]);
        if (((i + 1) % 8) == 0) {
            ez_printf("\r\n");
        }
    }
    if ((i % 8) != 0) ez_printf("\r\n");
}

/* 系统微秒级延时 */
#include <stdint.h>

#include "cmsis_compiler.h"  // for __NOP()

// #pragma GCC push_options
// #pragma GCC optimize("O0")
void delay_us(uint32_t us) {
    // 16MHZ need 16-2 __NOP(); 32MHZ need 32-2 __NOP();
    for (; us > 0; us--) {
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        //__NOP(); // minus two sys clock
        //__NOP();
    }
}
// #pragma GCC pop_options
