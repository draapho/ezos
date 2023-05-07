/**
 * \file            drv_uart.h
 * \brief           uart driver header file
 *                  STM32官方库uart中断收发不实用. 因此改写为buffer先进先出模式.
 */

#ifndef DRV_UART_H
#define DRV_UART_H

#include <string.h>

#include "main.h"

/* typedef */
typedef struct {
    USART_TypeDef *uart;
    uint8_t *tx_buff; /* Pointer to UART Tx transfer Buffer */
    uint8_t *tx_head;
    uint8_t *tx_tail;
    uint16_t tx_max;  /* UART Tx Transfer size              */
    uint16_t tx_i;    /* UART Tx Transfer Counter           */
    uint8_t *rx_buff; /* Pointer to UART Rx transfer Buffer */
    uint8_t *rx_head;
    uint8_t *rx_tail;
    uint16_t rx_max; /* UART Rx Transfer size              */
    uint16_t rx_i;   /* UART Rx Transfer Counter           */
} uart_name_t;

/* variable */
extern uart_name_t UART_MCU;

/* function */
void uart_handle_irq(uart_name_t *ez_uart);  // UART 中断处理函数, 需要放到相应的 UART 中断中.
void uart_buff_init_all(void);               // 初始化所有 UART 的缓冲区. 如果发送缓冲区设置为 NULL, 则为阻塞发送
void uart_tx(uart_name_t *ez_uart, uint8_t *pdata, uint16_t size);
int32_t uart_rx(uart_name_t *ez_uart, uint8_t *pdata, uint16_t max_size);  // return: 0 no data; -1 overflow

__STATIC_INLINE void uart_tx_str(uart_name_t *ez_uart, char *pdata, uint16_t length) {
    if (length == 0) length = strlen(pdata);
    uart_tx(ez_uart, (uint8_t *)pdata, length);
}

int8_t uart_ready_sleep(uart_name_t *ez_uart);  // for stm32g0. 0 still busy; 1, ready enter into stop mode

#endif /* DRV_UART_H */
