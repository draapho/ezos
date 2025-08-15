/**
 * \file            drv_gpio_led.h
 * \brief           LED driver header file. 支持可调/呼吸/闪烁效果.
 */

#ifndef DRV_GPIO_LED_H
#define DRV_GPIO_LED_H

#include "drv_gpio_cfg.h"

/* led config */
#define LED_ON_LEVEL 1  // 0, 低电平点亮LED. 1, 高电平点亮LED
#define LED_ADVANCED    // 使能LED的高级功能, 包括闪烁和渐变效果

/* X Macro: 定义所有的LED名称和对应的GPIO口 */
/* 推荐在 drv_gpio_cfg.h 内统一配置 */
/*
#define DRV_LED_NAME_GPIO \
    X(LED_EXAMPLE, GPIOA, GPIO_PIN_5)
*/

#ifdef DRV_LED_NAME_GPIO

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_LED_NAME_GPIO
#undef X
        DRV_LED_NAME_END,
} led_name_t;

#define LED_MASK 0x10
#define BLED_MASK 0x20
#define FLED_MASK 0x40
#define LED_ON_MASK 0x01
typedef enum {
    LED_OFF = LED_MASK | 0x00,          // LED灯关闭状态
    LED_ON = LED_MASK | LED_ON_MASK,    // LED灯打开状态
    BLED_OFF = BLED_MASK | 0x00,        // 呼吸灯关闭状态
    BLED_ON = BLED_MASK | LED_ON_MASK,  // 呼吸灯打开状态
    FLED_OFF = FLED_MASK | 0x00,        // 闪烁灯低电平状态
    FLED_ON = FLED_MASK | LED_ON_MASK,  // 闪烁灯高电平状态
} led_status_t;

/* function */
void led_init_all(void);                       // 初始化所有的LED
void led_init(led_name_t led_name);            // 初始化指定的LED
void led_on(led_name_t led_name);              // LED打开
void led_off(led_name_t led_name);             // LED关闭
void led_toggle(led_name_t led_name);          // LED翻转
led_status_t led_status(led_name_t led_name);  // 读取当前LED状态

#ifdef LED_ADVANCED

void led_scan_1ms(void);                                                  // LED 1ms 定时扫描, 在1ms定时中断函数中调用
void bled_set(led_name_t led_name, uint8_t level, uint16_t gradient_ms);  // LED渐变设定亮度值和渐变速度. 0最亮, 0xFF熄灭. 渐变速度单位ms, 0为400ms默认值.
void bled_on(led_name_t led_name, uint16_t gradient_ms);                  // LED渐变打开
void bled_off(led_name_t led_name, uint16_t gradient_ms);                 // LED渐变关闭
void bled_toggle(led_name_t led_name, uint16_t gradient_ms);              // LED渐变翻转
void led_flash(led_name_t led_name, uint16_t time_ms, uint8_t times);     // LED闪烁, time_ms为闪烁周期, counter指定次数, =0 表示一直闪烁

#endif /* LED_ADVANCED */

#endif /* DRV_LED_NAME_GPIO */

#endif /* DRV_GPIO_LED_H */
