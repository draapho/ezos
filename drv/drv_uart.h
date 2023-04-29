/**
 * \file            drv_uart.h
 * \brief           uart driver header file
 */

#ifndef DRV_UART_H
#define DRV_UART_H

#include "main.h"
#include "string.h"

/* typedef */
typedef struct {
    USART_TypeDef *USARTx;
    uint8_t *txBuff; /* Pointer to UART Tx transfer Buffer */
    uint8_t *txHead;
    uint8_t *txTail;
    uint16_t txMax;  /* UART Tx Transfer size              */
    uint16_t txN;    /* UART Tx Transfer Counter           */
    uint8_t *rxBuff; /* Pointer to UART Rx transfer Buffer */
    uint8_t *rxHead;
    uint8_t *rxTail;
    uint16_t rxMax; /* UART Rx Transfer size              */
    uint16_t rxN;   /* UART Rx Transfer Counter           */
} uart_name_t;

/* variable */
extern uart_name_t UART_MCU;

/* function */
void EZOS_UART_IRQHandler(uart_name_t *ez_uart);  // UART 中断处理函数, 需要放到相应的 UART 中断中, 如在 stm32xxx_it.c 文件.
void uart_buff_init_all(void);
void uart_tx(uart_name_t *ez_uart, uint8_t *pData, uint16_t size);
int32_t uart_rx(uart_name_t *ez_uart, uint8_t *pData, uint16_t max_size);  // return 0, no data; -1, overflow

__STATIC_INLINE void uart_tx_str(uart_name_t *ez_uart, char *pdata, uint16_t length) {
    if (length == 0) length = strlen(pdata);
    uart_tx(ez_uart, (uint8_t *)pdata, length);
}

#endif /* DRV_UART_H */
