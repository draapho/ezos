/**
 * \file            drv_gpio_key.h
 * \brief           KEY driver header file. 仅支持单按键的判断
 */

#ifndef DRV_GPIO_KEY_H
#define DRV_GPIO_KEY_H

#include "drv_gpio_cfg.h"

/* key config */
typedef uint16_t keyint_t;      // 数据类型的bit数, 确定了按键数量的最大值
#define KEY_CLICK_LEVEL 0       // 0, 低电平按键触发. 1, 高电平按键触发
#define KEY_SCAN_TICK_MS 10     // 按键扫描周期, 单位ms. 推荐 10-20ms.
#define KEY_LONG_PRESS_MS 1300  // 长按触发时间设置, 单位ms, 经测试使用1.3S较好
#define KEY_RUN_PRESS_MS 200    // 连发时间间隔设置, 单位ms, 经测试使用0.2s较好

// 选择需要支持的按键功能, 仅可节省非常少的资源.
#define KEY_TOUCH 0x01
#define KEY_CLICK 0x02
#define KEY_LCLICK 0x04
#define KEY_LPRESS 0x08
#define KEY_SUPPORT (KEY_TOUCH | KEY_CLICK | KEY_LCLICK | KEY_LPRESS)

/* X Macro: 定义所有的KEY名称和对应的GPIO口 */
/* 推荐在 drv_gpio_cfg.h 内统一配置 */
/*
#define DRV_KEY_NAME_GPIO \
    X(KEY_EXAMPLE, GPIOC, GPIO_PIN_13)
*/

#ifdef DRV_KEY_NAME_GPIO

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_KEY_NAME_GPIO
#undef X
        DRV_KEY_NAME_END,
    DRV_KEY_ALL = DRV_KEY_NAME_END,
} key_name_t;

typedef struct
{
    keyint_t on_off;  // 当前按键开关值
#if (KEY_SUPPORT & KEY_TOUCH)
    keyint_t touch;  // 按键按下时的事件
#endif
#if (KEY_SUPPORT & KEY_CLICK)
    keyint_t click;  // 按键快速释放事件
#endif
#if (KEY_SUPPORT & KEY_LCLICK)
    keyint_t lclick;  // 按键长按触发事件
#endif
#if (KEY_SUPPORT & KEY_LPRESS)
    keyint_t lpress;  // 按键连发触发事件
#endif
} key_event_t;

#define KEY_MASK(key_name) (1 << key_name)  // 按键取模, 对应到bit

/* variable */
/**
 * @brief 使用全局变量的方式提供直按式按键返回值
 *   @arg key_event.on_off,  当前按键开关值. 对应bit. 0,无按键. 1,有按键
 *   @arg key_event.touch,   按键按下时的事件, 对应bit置位, 不会自动置零
 *   @arg key_event.click,   按键快速释放事件, 对应bit置位, 不会自动置零
 *   @arg key_event.lclick,  按键长按触发事件, 对应bit置位, 不会自动置零
 *   @arg key_event.lpress,  按键连发效果事件. 对应bit置位, 不会自动置零
 *
 * @note 按键与上述参数的对应关系由 keyName_t 的枚举值决定按键bit位, 最多16个按键
 *       按键0示例：
 * ______________-----------------------------------------------------_____________________________
 *     |         |<- 0.2S ->|<-    1.1S   ->|<- 0.2S ->|<- 0.2 S->|         |           |
 *  无按键     按下                        按着      按着      按着          释放       无按键
 * .on_off=0     .on_off=1  .on_off=1         .on_off=1 .on_off=1          .on_off=0   .on_off=0
 * .touch=0      .touch=1
 * .click=0                 .click=1(若1.3S内释放)
 * .lclick=0                               .lclick=1
 * .lpress=0                               .lpress=1   .lpress=1   .lpress=1
 */
volatile extern key_event_t key_event;

/* function */
void key_init_all(void);             // 初始化所有按键
void key_init(key_name_t key_name);  // 初始化指定按键
void key_scan(void);                 // 按键扫描函数, 建议放在低优先级任务中. 调用周期和 KEY_SCAN_TICK_MS 基本一致.

__STATIC_INLINE keyint_t key_get(volatile keyint_t *event, key_name_t key_name) {
    return *event & KEY_MASK(key_name);
}

// if DRV_KEY_ALL, delete whole event
__STATIC_INLINE keyint_t key_clear(volatile keyint_t *event, key_name_t key_name) {
    keyint_t value;
    if (key_name < DRV_KEY_NAME_END) {
        value = *event & KEY_MASK(key_name);
        *event &= ~KEY_MASK(key_name);
    } else {
        value = *event;
        *event = 0;
    }
    return value;
}

__STATIC_INLINE void key_clear_all(void) {
    memset((void *)&key_event, 0, sizeof(key_event));
}

#endif /* DRV_KEY_NAME_GPIO */

#endif /* DRV_GPIO_KEY_H */
