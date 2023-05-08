/**
 * \file            drv_gpio_ctrl.h
 * \brief           Control port driver header file.
 */

#ifndef DRV_GPIO_CTRL_H
#define DRV_GPIO_CTRL_H

#include "drv_gpio_cfg.h"

/* ctrl config */
/* X Macro: 定义所有的控制端口, 对应的GPIO口, 以及关闭电平 */
/* 推荐在 drv_gpio_cfg.h 内统一配置 */
#ifndef DRV_CTRL_NAME_GPIO_ON
#define DRV_CTRL_NAME_GPIO_ON \
    X(CTRL_EXAMPLE, GPIOA, GPIO_PIN_5, 0)
#endif /* DRV_LED_NAME_GPIO */

/* typedef */
typedef enum {
#define X(name, ...) name,
    DRV_CTRL_NAME_GPIO_ON
#undef X
        DRV_CTRL_NAME_END,
} ctrl_name_t;

/* function */

void ctrl_init_all(void);                    // 初始化所有的控制引脚
void ctrl_init(ctrl_name_t ctrl_name);       // 初始化指定的控制引脚
void ctrl_on(ctrl_name_t ctrl_name);         // 控制引脚逻辑开
void ctrl_off(ctrl_name_t ctrl_name);        // 控制引脚逻辑关
void ctrl_toggle(ctrl_name_t ctrl_name);     // 控制引脚翻转
uint8_t ctrl_status(ctrl_name_t ctrl_name);  // 读取控制引脚的逻辑状态. 0, 关闭. 1, 打开

#endif /* DRV_GPIO_CTRL_H */
