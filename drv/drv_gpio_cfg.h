/**
 * \file            drv_gpio_cfg.h
 * \brief           driver cfg gpio port header file.
 */

#ifndef DRV_GPIO_CFG_H
#define DRV_GPIO_CFG_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ezos_def.h"  // for __STATIC_INLINE
#include "ezos_log.h"  // for LOG and ASSERT
#include "main.h"      // for port and pin type

/* CTRL config: 定义所有的控制端口, 对应的GPIO口, 以及打开电平 */
#define DRV_CTRL_NAME_GPIO_ON \
    X(LED, GPIOA, LL_GPIO_PIN_5, 1)

/* KEY config: 定义所有的按键名称和对应的GPIO口 */
#define DRV_KEY_NAME_GPIO \
    X(B1, GPIOC, LL_GPIO_PIN_13)

/* LED config: 定义所有的LED名称和对应的GPIO口 */
#define DRV_LED_NAME_GPIO \
    X(LD2, GPIOA, LL_GPIO_PIN_5)

/* test config */
#define TEST_ARGV_LEN_MAX 10  // 测试用的传入参数字符串的最大值
#define DRV_GPIO_TEST         // 是否打开 gpio 相关驱动的测试函数
#ifndef LOG
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#ifndef ASSERT
#define ASSERT(expr) while (!(expr))
#endif

/* gpio port */
typedef struct {
    void *const port;
    const uint16_t pin;
} gpio_hw_t;

// 初始化为输入口
__STATIC_INLINE void drv_input_init(const gpio_hw_t *io) {
    // GPIO_InitTypeDef gpio = {0};
    // gpio.Pin = io->pin;
    // gpio.Mode = GPIO_MODE_INPUT;
    // HAL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);

    LL_GPIO_InitTypeDef gpio = {0};
    gpio.Pin = io->pin;
    gpio.Mode = LL_GPIO_MODE_INPUT;
    LL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);
}

// 读取输入口当前电平
__STATIC_INLINE uint32_t drv_input_level(const gpio_hw_t *io) {
    return ((GPIO_TypeDef *)(io->port))->IDR & io->pin;
}

// 初始化为推挽输出口
__STATIC_INLINE void drv_output_pp_init(const gpio_hw_t *io) {
    // GPIO_InitTypeDef gpio = {0};
    // gpio.Pin = io->pin;
    // gpio.Mode = GPIO_MODE_OUTPUT_PP;
    // HAL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);

    LL_GPIO_InitTypeDef gpio = {0};
    gpio.Pin = io->pin;
    gpio.Mode = LL_GPIO_MODE_OUTPUT;
    gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);
}

// 初始化为开漏输出口
__STATIC_INLINE void drv_output_od_init(const gpio_hw_t *io) {
    // GPIO_InitTypeDef gpio = {0};
    // gpio.Pin = io->pin;
    // gpio.Mode = GPIO_MODE_OUTPUT_OD;
    // gpio.Pull = GPIO_NOPULL;  // 已有外部上拉
    // // gpio.Pull = GPIO_PULLUP;  // 使用内部上拉
    // HAL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);

    LL_GPIO_InitTypeDef gpio = {0};
    gpio.Pin = io->pin;
    gpio.Mode = LL_GPIO_MODE_OUTPUT;
    gpio.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio.Pull = LL_GPIO_PULL_NO;  // 已有外部上拉
    // gpio.Pull = LL_GPIO_PULL_UP;  // 使用内部上拉
    LL_GPIO_Init((GPIO_TypeDef *)(io->port), &gpio);
}

// 读取输出口当前电平
__STATIC_INLINE uint32_t drv_output_level(const gpio_hw_t *io) {
    return ((GPIO_TypeDef *)(io->port))->ODR & io->pin;
}

// 输出口电平置高
__STATIC_INLINE void drv_output_high(const gpio_hw_t *io) {
    ((GPIO_TypeDef *)(io->port))->BSRR = io->pin;
    // LL_GPIO_SetOutputPin((GPIO_TypeDef *)(io->port), io->pin);
    // HAL_GPIO_WritePin((GPIO_TypeDef *)(io->port), io->pin, GPIO_PIN_SET);
}

// 输出口电平置低
__STATIC_INLINE void drv_output_low(const gpio_hw_t *io) {
    ((GPIO_TypeDef *)(io->port))->BRR = io->pin;
    // (GPIO_TypeDef *)(io->port)->BSRR = io->pin << 16 ;
    // LL_GPIO_ResetOutputPin((GPIO_TypeDef *)(io->port), io->pin);
    // HAL_GPIO_WritePin((GPIO_TypeDef *)(io->port), io->pin, GPIO_PIN_RESET);
}

// 输出口电平翻转
__STATIC_INLINE void drv_output_toggle(const gpio_hw_t *io) {
    ((GPIO_TypeDef *)(io->port))->ODR ^= io->pin;
    // drv_output_level() ? drv_gpio_low() : drv_gpio_high();
}

/* delay function port*/
extern void delay_us(uint32_t us);  // 需要实现us阻塞延时函数

#endif /* DRV_GPIO_CFG_H */
