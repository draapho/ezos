/**
 * \file            drv_uart.c
 * \brief           uart driver for stm32l1 source file
 */

#include <stddef.h>

#include "drv_uart.h"

static void uart_buf_init(uart_name_t *ez_uart, USART_TypeDef *uart, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size);

/* uart porting */
uart_name_t UART_MCU;  // 定义UART名称

// 初始化所有 UART 的缓冲区. 如果发送缓冲区设置为 NULL, 则为阻塞发送
void uart_buff_init_all(void) {
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
void uart_handle_irq(uart_name_t *ez_uart) {
    USART_TypeDef *uart = ez_uart->uart;
    uint32_t isrflags = READ_REG(uart->SR);
    uint32_t cr1its = READ_REG(uart->CR1);
    uint32_t cr3its = READ_REG(uart->CR3);
    uint32_t errorflags;
    uint8_t data;

    /* error interrupt occurred ------------------------------------*/
    errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
    if (errorflags != 0U) {
        LL_USART_ClearFlag_PE(uart);
        LL_USART_ClearFlag_FE(uart);
        LL_USART_ClearFlag_NE(uart);
        if (((isrflags & USART_SR_ORE) != 0U) && (((cr1its & USART_CR1_RXNEIE) != 0U) || ((cr3its & USART_CR3_EIE) != 0U))) {
            LL_USART_ClearFlag_ORE(uart);
            ez_uart->rx_i = -1;
        }
    }

    /* UART in mode Receiver ---------------------------------------------------*/
    if (((isrflags & USART_SR_RXNE) != 0U) && ((cr1its & USART_CR1_RXNEIE) != 0U)) {
        data = (uint8_t)(uart->DR & (uint8_t)0x00FF);
        if (ez_uart->rx_i < ez_uart->rx_max) {
            *ez_uart->rx_tail++ = data;
            if (ez_uart->rx_tail >= (ez_uart->rx_buff + ez_uart->rx_max))
                ez_uart->rx_tail = ez_uart->rx_buff;
            ez_uart->rx_i++;
        } else {
            ez_uart->rx_i = -1;  // MAX value
        }
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if (((isrflags & USART_SR_TXE) != 0U) && ((cr1its & USART_CR1_TXEIE) != 0U)) {
        uart->DR = *ez_uart->tx_tail++;
        if (ez_uart->tx_tail >= (ez_uart->tx_buff + ez_uart->tx_max))
            ez_uart->tx_tail = ez_uart->tx_buff;
        ez_uart->tx_i--;
        if (ez_uart->tx_i == 0) {
            CLEAR_BIT(uart->CR1, USART_CR1_TXEIE);
            SET_BIT(uart->CR1, USART_CR1_TCIE);
        }
    }

    /* UART in mode Transmitter (transmission end) -----------------------------*/
    if (((isrflags & USART_SR_TC) != 0U) && ((cr1its & USART_CR1_TCIE) != 0U)) {
        CLEAR_BIT(uart->CR1, USART_CR1_TCIE);
    }
}

// 设置uart的收发缓冲区. 如果发送缓冲区设置为 NULL, 则为阻塞发送
void uart_buf_init(uart_name_t *ez_uart, USART_TypeDef *uart, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size) {
    CLEAR_BIT(uart->CR1, USART_CR1_RXNEIE);
    CLEAR_BIT(uart->CR3, USART_CR3_EIE);

    ez_uart->uart = uart;

    ez_uart->tx_buff = tx_data;
    ez_uart->tx_head = tx_data;
    ez_uart->tx_tail = tx_data;
    ez_uart->tx_max = tx_size;
    ez_uart->tx_i = 0;

    ez_uart->rx_buff = rx_data;
    ez_uart->rx_head = rx_data;
    ez_uart->rx_tail = rx_data;
    ez_uart->rx_max = rx_size;
    ez_uart->rx_i = 0;

    SET_BIT(uart->CR3, USART_CR3_EIE);
    SET_BIT(uart->CR1, USART_CR1_RXNEIE);
}

// return: 0 no data; -1 overflow
int32_t uart_rx(uart_name_t *ez_uart, uint8_t *pdata, uint16_t max_size) {
    int32_t i;

    if ((ez_uart == NULL) || (ez_uart->rx_i == 0))
        return 0;

    CLEAR_BIT(ez_uart->uart->CR1, USART_CR1_RXNEIE);
    if (ez_uart->rx_i <= ez_uart->rx_max) {  // not overflow
        for (i = 0; (i < max_size) && (i < ez_uart->rx_i); i++) {
            if (ez_uart->rx_head >= (ez_uart->rx_buff + ez_uart->rx_max)) {
                ez_uart->rx_head = ez_uart->rx_buff;
            }
            *pdata++ = *ez_uart->rx_head++;
        }
        ez_uart->rx_i -= i;
    } else {  // overflow
        ez_uart->rx_head = ez_uart->rx_tail = ez_uart->rx_buff;
        ez_uart->rx_i = 0;
        i = -1;
    }
    SET_BIT(ez_uart->uart->CR1, USART_CR1_RXNEIE);
    return i;
}

void uart_tx(uart_name_t *ez_uart, uint8_t *pdata, uint16_t size) {
    uint16_t i = 0;
    if ((ez_uart->tx_buff == NULL) || (ez_uart->tx_max == 0)) {  // 阻塞发送
        for (i = 0; i < size; i++) {
            while (!LL_USART_IsActiveFlag_TXE(ez_uart->uart)) {
            }
            LL_USART_TransmitData8(ez_uart->uart, *pdata++);
        }
        while (!LL_USART_IsActiveFlag_TC(ez_uart->uart)) {
        }
    } else {  // 非阻塞发送
        while (1) {
            if (size == 0) return;
            for (i = 0; i < size; i++) {
                if (i >= (ez_uart->tx_max - ez_uart->tx_i)) break;
                if (ez_uart->tx_head >= (ez_uart->tx_buff + ez_uart->tx_max)) {  // pHead loop
                    ez_uart->tx_head = ez_uart->tx_buff;
                }
                *ez_uart->tx_head++ = *pdata++;
            }
            ez_uart->tx_i += i;
            size -= i;
            if (LL_USART_IsEnabledIT_TXE(ez_uart->uart) == 0U)
                LL_USART_EnableIT_TXE(ez_uart->uart);
        }
    }
}
