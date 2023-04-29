/**
 * \file            drv_uart.c
 * \brief           uart driver for stm32L0 source file
 */

#include <stddef.h>

#include "drv_uart.h"

static void uart_buf_init(uart_name_t *ez_uart, USART_TypeDef *USARTx, uint8_t *txData, uint16_t txSize, uint8_t *rxData, uint16_t rxSize);

/* uart porting */
uart_name_t UART_MCU;  // 定义UART名称

void uart_buff_init_all(void) {  // 初始化所有 UART 的缓冲区
#define BUFF_SIZE_DEFAULT 0xFF

    /* 配置为缓冲发送 */
    //  static uint8_t txbuf2[BUFF_SIZE_DEFAULT], rxbuf2[BUFF_SIZE_DEFAULT];
    //  uart_buf_init(&UART_MCU, USART2, txbuf2, sizeof(txbuf2), rxbuf2, sizeof(rxbuf2));

    /* 配置为阻塞发送 */
    static uint8_t rxbuf2[BUFF_SIZE_DEFAULT];
    uart_buf_init(&UART_MCU, USART2, NULL, 0, rxbuf2, sizeof(rxbuf2));
}

/* function */
// UART 中断处理函数, 需要放到相应的 UART 中断中.
void EZOS_UART_IRQHandler(uart_name_t *ez_uart) {
    USART_TypeDef *USARTx = ez_uart->USARTx;
    uint32_t isrflags = READ_REG(USARTx->SR);
    uint32_t cr1its = READ_REG(USARTx->CR1);
    uint32_t cr3its = READ_REG(USARTx->CR3);
    uint32_t errorflags;
    uint8_t data;

    /* error interrupt occurred ------------------------------------*/
    errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
    if (errorflags != 0U) {
        LL_USART_ClearFlag_PE(USARTx);
        LL_USART_ClearFlag_FE(USARTx);
        LL_USART_ClearFlag_NE(USARTx);
        if (((isrflags & USART_SR_ORE) != 0U) && (((cr1its & USART_CR1_RXNEIE) != 0U) || ((cr3its & USART_CR3_EIE) != 0U))) {
            LL_USART_ClearFlag_ORE(USARTx);
            ez_uart->rxN = -1;
        }
    }

    /* UART in mode Receiver ---------------------------------------------------*/
    if (((isrflags & USART_SR_RXNE) != 0U) && ((cr1its & USART_CR1_RXNEIE) != 0U)) {
        data = (uint8_t)(USARTx->DR & (uint8_t)0x00FF);
        if (ez_uart->rxN < ez_uart->rxMax) {
            *ez_uart->rxTail++ = data;
            if (ez_uart->rxTail >= (ez_uart->rxBuff + ez_uart->rxMax))
                ez_uart->rxTail = ez_uart->rxBuff;
            ez_uart->rxN++;
        } else {
            ez_uart->rxN = -1;  // MAX value
        }
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if (((isrflags & USART_SR_TXE) != 0U) && ((cr1its & USART_CR1_TXEIE) != 0U)) {
        USARTx->DR = *ez_uart->txTail++;
        if (ez_uart->txTail >= (ez_uart->txBuff + ez_uart->txMax))
            ez_uart->txTail = ez_uart->txBuff;
        ez_uart->txN--;
        if (ez_uart->txN == 0) {
            CLEAR_BIT(USARTx->CR1, USART_CR1_TXEIE);
            SET_BIT(USARTx->CR1, USART_CR1_TCIE);
        }
    }

    /* UART in mode Transmitter (transmission end) -----------------------------*/
    if (((isrflags & USART_SR_TC) != 0U) && ((cr1its & USART_CR1_TCIE) != 0U)) {
        CLEAR_BIT(USARTx->CR1, USART_CR1_TCIE);
    }
}

void uart_buf_init(uart_name_t *ez_uart, USART_TypeDef *USARTx, uint8_t *txData, uint16_t txSize, uint8_t *rxData, uint16_t rxSize) {
    CLEAR_BIT(USARTx->CR1, USART_CR1_RXNEIE);
    CLEAR_BIT(USARTx->CR3, USART_CR3_EIE);

    ez_uart->USARTx = USARTx;

    ez_uart->txBuff = txData;
    ez_uart->txHead = txData;
    ez_uart->txTail = txData;
    ez_uart->txMax = txSize;
    ez_uart->txN = 0;

    ez_uart->rxBuff = rxData;
    ez_uart->rxHead = rxData;
    ez_uart->rxTail = rxData;
    ez_uart->rxMax = rxSize;
    ez_uart->rxN = 0;

    SET_BIT(USARTx->CR3, USART_CR3_EIE);
    SET_BIT(USARTx->CR1, USART_CR1_RXNEIE);
}

// return 0, no data, -1, overflow
int32_t uart_rx(uart_name_t *ez_uart, uint8_t *pData, uint16_t max_size) {
    int32_t i;

    if ((ez_uart == NULL) || (ez_uart->rxN == 0))
        return 0;

    CLEAR_BIT(ez_uart->USARTx->CR1, USART_CR1_RXNEIE);
    if (ez_uart->rxN <= ez_uart->rxMax) {  // not overflow
        for (i = 0; (i < max_size) && (i < ez_uart->rxN); i++) {
            if (ez_uart->rxHead >= (ez_uart->rxBuff + ez_uart->rxMax)) {
                ez_uart->rxHead = ez_uart->rxBuff;
            }
            *pData++ = *ez_uart->rxHead++;
        }
        ez_uart->rxN -= i;
    } else {  // overflow
        ez_uart->rxHead = ez_uart->rxTail = ez_uart->rxBuff;
        ez_uart->rxN = 0;
        i = -1;
    }
    SET_BIT(ez_uart->USARTx->CR1, USART_CR1_RXNEIE);
    return i;
}

void uart_tx(uart_name_t *ez_uart, uint8_t *pData, uint16_t size) {
    uint16_t i = 0;
    if ((ez_uart->txBuff == NULL) || (ez_uart->txMax == 0)) {  // 阻塞发送
        for (i = 0; i < size; i++) {
            while (!LL_USART_IsActiveFlag_TXE(ez_uart->USARTx)) {
            }
            LL_USART_TransmitData8(ez_uart->USARTx, *pData++);
        }
        while (!LL_USART_IsActiveFlag_TC(ez_uart->USARTx)) {
        }
    } else {  // 非阻塞发送
        while (1) {
            if (size == 0) return;
            for (i = 0; i < size; i++) {
                if (i >= (ez_uart->txMax - ez_uart->txN)) break;
                if (ez_uart->txHead >= (ez_uart->txBuff + ez_uart->txMax)) {  // pHead loop
                    ez_uart->txHead = ez_uart->txBuff;
                }
                *ez_uart->txHead++ = *pData++;
            }
            ez_uart->txN += i;
            size -= i;
            if (LL_USART_IsEnabledIT_TXE(ez_uart->USARTx) == 0U)
                LL_USART_EnableIT_TXE(ez_uart->USARTx);
        }
    }
}
