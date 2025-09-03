/**
 * \file            drv_gpio_i2c.h
 * \brief           I2C driver header file. IO口模拟 I2C 驱动程序. 主机模式, 7位地址.
 */

#ifndef DRV_GPIO_I2C_H
#define DRV_GPIO_I2C_H

#include "drv_gpio_cfg.h"

/* i2c config */
/* X Macro: 定义所有的I2C名称和SCL,SDA的GPIO口 */
/* 推荐在 drv_gpio_cfg.h 内统一配置 */
/*
#define DRV_I2C_NAME_SCL_SDA \
    X(I2C_EXAMPLE, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7)
*/

#ifdef DRV_I2C_NAME_SCL_SDA

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_I2C_NAME_SCL_SDA
#undef X
        DRV_I2C_NAME_END,
} i2c_name_t;

typedef enum {
    I2C_ACK = 0,     // I2C 有应答信号
    I2C_NO_ACK = 1,  // I2C 无应答信号
} i2c_ack_t;

/* function */
void i2c_init(i2c_name_t i2c_name, uint16_t baudrate_khz);                // I2C初始化
void i2c_start(i2c_name_t i2c_name);                                      // 产生I2C起始信号
void i2c_stop(i2c_name_t i2c_name);                                       // 产生I2C结束信号
i2c_ack_t i2c_tx_addr_wr_slaver(i2c_name_t i2c_name, uint8_t addr_bit0);  // I2C准备写从设备
i2c_ack_t i2c_tx_addr_rd_slaver(i2c_name_t i2c_name, uint8_t addr_bit0);  // I2C准备读从设备
i2c_ack_t i2c_tx_byte(i2c_name_t i2c_name, uint8_t data);                 // 通过I2C发送一个字节
uint8_t i2c_rx_byte(i2c_name_t i2c_name, i2c_ack_t ack);                  // 通过I2C接收一个字节

#endif /* DRV_I2C_NAME_SCL_SDA */

#endif /* DRV_GPIO_I2C_H */