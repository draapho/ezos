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
#ifndef DRV_LED_NAME_GPIO
#define DRV_LED_NAME_GPIO \
    X(LED_EXAMPLE, GPIOA, GPIO_PIN_5)
#endif /* DRV_LED_NAME_GPIO */

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_LED_NAME_GPIO
#undef X
        DRV_LED_NAME_END,
} led_name_t;

typedef enum {
    LED_OFF = 0x10,   // LED灯关闭状态
    LED_ON = 0x11,    // LED灯打开状态
    BLED_OFF = 0x20,  // 呼吸灯关闭状态
    BLED_ON = 0x21,   // 呼吸灯打开状态
    FLED_OFF = 0x40,  // 闪烁灯低电平状态
    FLED_ON = 0x41,   // 闪烁灯高电平状态
    LED_ON_MASK = 0x01,
    LED_MASK = 0x10,
    BLED_MASK = 0x20,
    FLED_MASK = 0x40,
} led_status_t;

/* function */
void led_init_all(void);                       // 初始化所有的LED
void led_init(led_name_t led_name);            // 初始化指定的LED
void led_on(led_name_t led_name);              // LED打开
void led_off(led_name_t led_name);             // LED关闭
void led_toggle(led_name_t led_name);          // LED翻转
led_status_t led_status(led_name_t led_name);  // 读取当前LED状态

#ifdef LED_ADVANCED

void led_scan_1ms(void);                                                 // LED 1ms 定时扫描, 在1ms定时中断函数中调用
void bled_set(led_name_t led_name, uint8_t level);                       // LED渐变设定亮度值. 0最亮, 0xFF熄灭.
void bled_on(led_name_t led_name);                                       // LED渐变打开
void bled_off(led_name_t led_name);                                      // LED渐变关闭
void bled_toggle(led_name_t led_name);                                   // LED渐变翻转
void led_flash(led_name_t led_name, uint16_t time_ms, uint8_t counter);  // LED闪烁, time_ms为闪烁周期, counter指定次数, =0 表示一直闪烁

#endif /* LED_ADVANCED */

#endif /* DRV_GPIO_LED_H */
