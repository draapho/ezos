/**
 * \file            drv_gpio_spi.c
 * \brief           SPI driver source file. IO口模拟 SPI 驱动程序. 主机模式.
 */

#include "drv_gpio_spi.h"

#ifdef DRV_SPI_NAME_SCK_MISO_MOSI

/* spi porting */
typedef struct {
    gpio_hw_t sck;
    gpio_hw_t miso;
    gpio_hw_t mosi;
} spi_hw_t;

static const spi_hw_t spi_hw[DRV_SPI_NAME_END] = {  // SPI硬件映射表
#define X(name, cport, cpin, iport, ipin, oport, opin) {{cport, cpin}, {iport, ipin}, {oport, opin}},
    DRV_SPI_NAME_SCK_MISO_MOSI
#undef X
};

// SPI微秒延时函数, 用于产生软件模拟SPI的SCK信号波特率.
__STATIC_INLINE void spi_delay_us(int16_t us) {
    delay_us(us);
}

// SPI软件模拟 IO口初始化
__STATIC_INLINE void spi_port_init(spi_name_t spi_name) {
#ifndef DRV_SPI_2_WIRES  // 三线
    drv_output_pp_init(&spi_hw[spi_name].sck);
    drv_output_pp_init(&spi_hw[spi_name].mosi);
    drv_input_init(&spi_hw[spi_name].miso);
#else  // 二线
    drv_output_pp_init(&spi_hw[spi_name].sck);
    // drv_output_od_init(&spi_hw[spi_name].mosi);
    // drv_output_od_init(&spi_hw[spi_name].miso);
#endif /* DRV_SPI_2_WIRES */
}

// SPI软件模拟SCK电平置高
__STATIC_INLINE void spi_sck_high(spi_name_t spi_name) {
    drv_output_high(&spi_hw[spi_name].sck);
}

// SPI软件模拟SCK电平置低
__STATIC_INLINE void spi_sck_low(spi_name_t spi_name) {
    drv_output_low(&spi_hw[spi_name].sck);
}

// SPI软件模拟MOSI电平置高
__STATIC_INLINE void spi_mosi_high(spi_name_t spi_name) {
    drv_output_high(&spi_hw[spi_name].mosi);
}

// SPI软件模拟MOSI电平置低
__STATIC_INLINE void spi_mosi_low(spi_name_t spi_name) {
    drv_output_low(&spi_hw[spi_name].mosi);
}

// 读取软件模拟SPI的MISO电平值
__STATIC_INLINE uint32_t spi_miso_level(spi_name_t spi_name) {
    return drv_input_level(&spi_hw[spi_name].miso);
}

/* variable */
typedef struct {
    uint8_t type;  // 软件模拟SPI用, 记录SPI传输类型
    int16_t us;    // 软件模拟SPI用, 记录延时时间, 单位us.
} spi_para_t;

static spi_para_t spi_para[DRV_SPI_NAME_END];

/* function */
/**
 ******************************************************************************
 * @brief  SPI初始化函数, 必须在初始化完成后再配置SPI的SS引脚
 * @param  spi_name, SPI 的宏定义名称
 * @param  spi_type, 设置SPI的传输类型
 *   @arg \c  SPI_TYPE_HIGH_EDGE1_MSB, SCK默认高电平, 下降沿检测, MSB模式
 *   @arg \c  SPI_TYPE_HIGH_EDGE1_LSB, SCK默认高电平, 下降沿检测, LSB模式
 *   @arg \c  SPI_TYPE_HIGH_EDGE2_MSB, SCK默认高电平, 上升沿检测, MSB模式
 *   @arg \c  SPI_TYPE_HIGH_EDGE2_LSB, SCK默认高电平, 上升沿检测, LSB模式
 *   @arg \c  SPI_TYPE_LOW_EDGE1_MSB,  SCK默认低电平, 上升沿检测, MSB模式
 *   @arg \c  SPI_TYPE_LOW_EDGE1_LSB,  SCK默认低电平, 上升沿检测, LSB模式
 *   @arg \c  SPI_TYPE_LOW_EDGE2_MSB,  SCK默认低电平, 下降沿检测, MSB模式
 *   @arg \c  SPI_TYPE_LOW_EDGE2_LSB,  SCK默认低电平, 下降沿检测, LSB模式
 * @param  baudrate_khz, 设置SPI波特率. 单位为khz.
 *   @arg     1-500,    软件模拟SPI波特率有效值 1-500 Khz.
 *   @arg     0, >500,  表示不延时. 能上Mhz, 取决于系统时钟.
 ******************************************************************************
 */
void spi_init(spi_name_t spi_name, spi_type_t spi_type, uint16_t baudrate_khz) {
    ASSERT(spi_name < DRV_SPI_NAME_END);
    ASSERT(IS_SPI_TYPE(spi_type));
    if (spi_name < DRV_SPI_NAME_END) {
        spi_port_init(spi_name);
        spi_mosi_low(spi_name);
        if (spi_type & 0x04)
            spi_sck_high(spi_name);
        else
            spi_sck_low(spi_name);

        spi_para[spi_name].type = (uint8_t)spi_type;  // 记录SPI模式和波特率
        if (baudrate_khz)
            spi_para[spi_name].us = (500 / baudrate_khz);  // 1000us 2等分为 500us, 进行时序控制
        else
            spi_para[spi_name].us = 0;
    }
}

// SPI收发一个字节
uint8_t spi_tx_rx_byte(spi_name_t spi_name, uint8_t send) {
    uint8_t i, receive = 0;
    int16_t delay;
    if (spi_name >= DRV_SPI_NAME_END) return 0;

    delay = spi_para[spi_name].us;
    switch (spi_para[spi_name].type) {
        case SPI_TYPE_HIGH_EDGE1_MSB: {
            for (i = 8; i > 0; i--) {
                if (send & 0x80)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send <<= 1;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                receive <<= 1;
                if (spi_miso_level(spi_name)) receive |= 0x01;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
            }
            break;
        }
        case SPI_TYPE_HIGH_EDGE1_LSB: {
            for (i = 8; i > 0; i--) {
                if (send & 0x01)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send >>= 1;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                receive >>= 1;
                if (spi_miso_level(spi_name)) receive |= 0x80;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
            }
            break;
        }
        case SPI_TYPE_HIGH_EDGE2_MSB: {
            for (i = 8; i > 0; i--) {
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                if (send & 0x80)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send <<= 1;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                receive <<= 1;
                if (spi_miso_level(spi_name)) receive |= 0x01;
            }
            break;
        }
        case SPI_TYPE_HIGH_EDGE2_LSB: {
            for (i = 8; i > 0; i--) {
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                if (send & 0x01)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send >>= 1;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                receive >>= 1;
                if (spi_miso_level(spi_name)) receive |= 0x80;
            }
            break;
        }
        case SPI_TYPE_LOW_EDGE1_MSB: {
            for (i = 8; i > 0; i--) {
                if (send & 0x80)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send <<= 1;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                receive <<= 1;
                if (spi_miso_level(spi_name)) receive |= 0x01;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
            }
            break;
        }
        case SPI_TYPE_LOW_EDGE1_LSB: {
            for (i = 8; i > 0; i--) {
                if (send & 0x01)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send >>= 1;
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                receive >>= 1;
                if (spi_miso_level(spi_name)) receive |= 0x80;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
            }
            break;
        }
        case SPI_TYPE_LOW_EDGE2_MSB: {
            for (i = 8; i > 0; i--) {
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                if (send & 0x80)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send <<= 1;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                receive <<= 1;
                if (spi_miso_level(spi_name)) receive |= 0x01;
            }
            break;
        }
        case SPI_TYPE_LOW_EDGE2_LSB: {
            for (i = 8; i > 0; i--) {
                spi_delay_us(delay);
                spi_sck_high(spi_name);
                if (send & 0x01)
                    spi_mosi_high(spi_name);
                else
                    spi_mosi_low(spi_name);
                send >>= 1;
                spi_delay_us(delay);
                spi_sck_low(spi_name);
                receive >>= 1;
                if (spi_miso_level(spi_name)) receive |= 0x80;
            }
            break;
        }
        default:
            receive = 0xFF;
            break;
    }
    return receive;
}

#endif /* DRV_SPI_NAME_SCK_MISO_MOSI */
