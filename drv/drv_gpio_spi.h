/**
 * \file            drv_gpio_i2c.c
 * \brief           SPI driver header file. IO口模拟 SPI 驱动程序. 主机模式.
 */

#ifndef DRV_GPIO_SPI_H
#define DRV_GPIO_SPI_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "drv_gpio_cfg.h"

/* spi config */
// #define DRV_SPI_2_WIRES  // 二线SPI(SCK, MISO和MOSI用同一个IO)

/* X Macro: 定义所有的SPI名称和SCK,MISO,MOSI的GPIO口 */
/* 推荐在 drv_gpio_cfg.h 内统一配置 */
/*
#define DRV_SPI_NAME_SCK_MISO_MOSI \
    X(SPI_EXAMPLE, GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_7)
*/

#ifdef DRV_SPI_NAME_SCK_MISO_MOSI

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_SPI_NAME_SCK_MISO_MOSI
#undef X
        DRV_SPI_NAME_END,
} spi_name_t;

typedef enum {
    SPI_TYPE_HIGH_EDGE1_MSB = (0x80 | 0x04 | 0x00 | 0x00),
    SPI_TYPE_HIGH_EDGE1_LSB = (0x80 | 0x04 | 0x00 | 0x01),
    SPI_TYPE_HIGH_EDGE2_MSB = (0x80 | 0x04 | 0x02 | 0x00),
    SPI_TYPE_HIGH_EDGE2_LSB = (0x80 | 0x04 | 0x02 | 0x01),
    SPI_TYPE_LOW_EDGE1_MSB = (0x80 | 0x00 | 0x00 | 0x00),
    SPI_TYPE_LOW_EDGE1_LSB = (0x80 | 0x00 | 0x00 | 0x01),
    SPI_TYPE_LOW_EDGE2_MSB = (0x80 | 0x00 | 0x02 | 0x00),
    SPI_TYPE_LOW_EDGE2_LSB = (0x80 | 0x00 | 0x02 | 0x01),
} spi_type_t;

#define IS_SPI_TYPE(spi_type) (((spi_type) == SPI_TYPE_HIGH_EDGE1_MSB) || \
                               ((spi_type) == SPI_TYPE_HIGH_EDGE1_LSB) || \
                               ((spi_type) == SPI_TYPE_HIGH_EDGE2_MSB) || \
                               ((spi_type) == SPI_TYPE_HIGH_EDGE2_LSB) || \
                               ((spi_type) == SPI_TYPE_LOW_EDGE1_MSB) ||  \
                               ((spi_type) == SPI_TYPE_LOW_EDGE1_LSB) ||  \
                               ((spi_type) == SPI_TYPE_LOW_EDGE2_MSB) ||  \
                               ((spi_type) == SPI_TYPE_LOW_EDGE2_LSB))

/* function */
/**
 ******************************************************************************
 * @brief  SPI初始化函数, 必须在初始化完成后再配置SPI的SS引脚
 * @param  spi_name, SPI 的宏定义名称
 * @param  spi_type, 设置SPI的传输类型
 *   @arg \c  SPI_TYPE_HIGH_EDGE1_MSB, SCK默认高电平, 下降沿检测, MSB模式
 *            SCK　       ----↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑----
 *            MOSI / MISO --BIT7---XXX---XXX---XXX---XXX---XXX---XXX---BIT0-----
 *
 *   @arg \c  SPI_TYPE_HIGH_EDGE1_LSB, SCK默认高电平, 下降沿检测, LSB模式
 *            SCK　       ----↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑----
 *            MOSI / MISO --BIT0---XXX---XXX---XXX---XXX---XXX---XXX---BIT7-----
 *
 *   @arg \c  SPI_TYPE_HIGH_EDGE2_MSB, SCK默认高电平, 上升沿检测, MSB模式
 *            SCK　       ----↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑----
 *            MOSI / MISO -----BIT7---XXX---XXX---XXX---XXX---XXX---XXX---BIT0--
 *
 *   @arg \c  SPI_TYPE_HIGH_EDGE2_LSB, SCK默认高电平, 上升沿检测, LSB模式
 *            SCK　       ----↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑----
 *            MOSI / MISO -----BIT0---XXX---XXX---XXX---XXX---XXX---XXX---BIT7--
 *
 *   @arg \c  SPI_TYPE_LOW_EDGE1_MSB,  SCK默认低电平, 上升沿检测, MSB模式
 *            SCK　       ____↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓____
 *            MOSI / MISO --BIT7---XXX---XXX---XXX---XXX---XXX---XXX---BIT0-----
 *
 *   @arg \c  SPI_TYPE_LOW_EDGE1_LSB,  SCK默认低电平, 上升沿检测, LSB模式
 *            SCK　       ____↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓____
 *            MOSI / MISO --BIT0---XXX---XXX---XXX---XXX---XXX---XXX---BIT7-----
 *
 *   @arg \c  SPI_TYPE_LOW_EDGE2_MSB,  SCK默认低电平, 下降沿检测, MSB模式
 *            SCK　       ____↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓____
 *            MOSI / MISO -----BIT7---XXX---XXX---XXX---XXX---XXX---XXX---BIT0--
 *
 *   @arg \c  SPI_TYPE_LOW_EDGE2_LSB,  SCK默认低电平, 下降沿检测, LSB模式
 *            SCK　       ____↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓__↑--↓____
 *            MOSI / MISO -----BIT0---XXX---XXX---XXX---XXX---XXX---XXX---BIT7--
 *
 * @param  baudrate_khz, 设置SPI波特率. 单位为khz.
 *   @arg     1-500,    软件模拟SPI波特率有效值 1-500 Khz.
 *   @arg     0,        表示不延时. 能上Mhz, 取决于系统时钟.
 ******************************************************************************
 */
void spi_init(spi_name_t spi_name, spi_type_t spi_type, uint16_t baudrate_khz);
uint8_t spi_tx_rx_byte(spi_name_t spi_name, uint8_t send);  // SPI收发一个字节

#endif /* DRV_SPI_NAME_SCK_MISO_MOSI */

#endif /* DRV_GPIO_SPI_H */
