/**
 * \file            drv_gpio_led.c
 * \brief           LED driver source file. 支持可调灯/呼吸灯效果.
 */

#include "drv_gpio_led.h"

#ifdef DRV_LED_NAME_GPIO

/* led porting */
static const gpio_hw_t led_hw[DRV_LED_NAME_END] = {  // LED硬件映射表
#define X(name, port, pin) {port, pin},
    DRV_LED_NAME_GPIO
#undef X
};

// LED IO口初始化
__STATIC_INLINE void led_port_init(led_name_t led_name) {
    drv_output_pp_init(&led_hw[led_name]);
}

// LED IO口电平置高
__STATIC_INLINE void led_port_high(led_name_t led_name) {
    drv_output_high(&led_hw[led_name]);
}

// LED IO口电平置低
__STATIC_INLINE void led_port_low(led_name_t led_name) {
    drv_output_low(&led_hw[led_name]);
}

// LED IO口电平翻转
__STATIC_INLINE void led_port_toggle(led_name_t led_name) {
    drv_output_toggle(&led_hw[led_name]);
}

// 读取 LED IO口电平值
__STATIC_INLINE uint32_t led_port_level(led_name_t led_name) {
    return drv_output_level(&led_hw[led_name]);
}

/* define and typedef */
#ifdef LED_ADVANCED
typedef struct
{
    uint16_t counter;      // LED PWM计数器/闪烁计数器
    uint16_t dutyfactor;   // LED PWM占空比/闪烁频率
    uint16_t destination;  // LED PWM占空比目标值/闪烁次数
    uint8_t times;         // LED 调整原本固定的渐变时间
    uint8_t timei;         // 计数渐变时间
} led_para_t;
#endif

#define FLAG_MASK 0xC000  // LED类型标记
#define FLAG_LED 0x0000   // 普通灯标记
#define FLAG_BLED 0x4000  // 呼吸灯标记
#define FLAG_FLED 0x8000  // 闪烁灯标记

#define BLED_PWM_WIDTH 20                                     // B=Breath, 呼吸灯PWM脉宽, 20ms 为 50hz, 无闪烁, 20级渐变, 梯度合适. 允许最小值: 16
#define BLED_GRADIENT_BASE (BLED_PWM_WIDTH * BLED_PWM_WIDTH)  // B=Breath, 呼吸灯渐变基础时间
#define FLED_TIME_MIN 20                                      // F=Flash,  闪烁周期最小值, 50hz
#define FLED_TIME_MAX 30000                                   // F=Flash,  闪烁周期最大值, 闪烁周期必须小于 0x4000 ms

/* variable */
#ifdef LED_ADVANCED
static volatile led_para_t led_para[DRV_LED_NAME_END];
#endif

/* function */
void led_init_all(void) {
    uint8_t i;
    for (i = 0; i < DRV_LED_NAME_END; i++)
        led_init((led_name_t)i);
}

void led_init(led_name_t led_name) {
    ASSERT(led_name < DRV_LED_NAME_END);
    if (led_name < DRV_LED_NAME_END) {
        led_port_init(led_name);
        led_off(led_name);
    }
}

void led_on(led_name_t led_name) {
    if (led_name < DRV_LED_NAME_END) {
#ifdef LED_ADVANCED
        led_para[led_name].counter = FLAG_LED | 0;
        led_para[led_name].dutyfactor = FLAG_LED | 0;
        led_para[led_name].destination = FLAG_LED | 0;
#endif

#if (LED_ON_LEVEL > 0)
        led_port_high(led_name);
#else
        led_port_low(led_name);
#endif
    }
}

void led_off(led_name_t led_name) {
    if (led_name < DRV_LED_NAME_END) {
#ifdef LED_ADVANCED
        led_para[led_name].counter = FLAG_LED | 0;
        led_para[led_name].dutyfactor = FLAG_LED | 0;
        led_para[led_name].destination = FLAG_LED | 0;
#endif

#if (LED_ON_LEVEL > 0)
        led_port_low(led_name);
#else
        led_port_high(led_name);
#endif
    }
}

void led_toggle(led_name_t led_name) {
    if (led_name < DRV_LED_NAME_END) {
#ifdef LED_ADVANCED
        led_para[led_name].counter = FLAG_LED | 0;
        led_para[led_name].dutyfactor = FLAG_LED | 0;
        led_para[led_name].destination = FLAG_LED | 0;
#endif
        led_port_toggle(led_name);
    }
}

led_status_t led_status(led_name_t led_name) {
    if (led_name >= DRV_LED_NAME_END) return LED_OFF;

#ifdef LED_ADVANCED
    if ((led_para[led_name].destination & FLAG_MASK) == FLAG_BLED) {  // 呼吸灯
        if (led_para[led_name].destination < (FLAG_BLED | BLED_PWM_WIDTH))
            return BLED_ON;
        else
            return BLED_OFF;
    }

    if ((led_para[led_name].destination & FLAG_MASK) == FLAG_FLED) {  // 闪烁灯
#if (LED_ON_LEVEL > 0)
        if (led_port_level(led_name))
            return FLED_ON;
        else
            return FLED_OFF;
#else
        if (led_port_level(led_name))
            return FLED_OFF;
        else
            return FLED_ON;
#endif /* LED_ON_LEVEL */
    }
#endif /* LED_ADVANCED */

#if (LED_ON_LEVEL > 0)  // 普通灯
    if (led_port_level(led_name))
        return LED_ON;
    else
        return LED_OFF;
#else
    if (led_port_level(led_name))
        return LED_OFF;
    else
        return LED_ON;
#endif /* LED_ON_LEVEL */
}

#ifdef LED_ADVANCED
// LED调节亮度值和渐变速度, 0最亮, 0xFF熄灭. 渐变速度: 只能实现400ms的倍数或整除的时间. 0表示使用默认值
void bled_set(led_name_t led_name, uint8_t level, uint16_t gradient_ms) {
    if (led_name < DRV_LED_NAME_END) {
        led_status_t st = led_status(led_name);
        level = (uint16_t)level * BLED_PWM_WIDTH / 0xFF;

        led_para[led_name].counter = FLAG_BLED | 0;  // 重新开始PWM计数, 避免中断后改变数值

        if (gradient_ms == 0) {  // 0, 使用默认渐变时间
            gradient_ms = BLED_GRADIENT_BASE;
        }
        if (gradient_ms >= BLED_GRADIENT_BASE) {
            led_para[led_name].timei = 0;
            led_para[led_name].times = (gradient_ms / BLED_GRADIENT_BASE);  // 渐变时间要拉长
        } else {
            uint8_t tmp = (BLED_GRADIENT_BASE / gradient_ms);
            if (tmp > BLED_PWM_WIDTH) tmp = BLED_PWM_WIDTH;
            led_para[led_name].timei = tmp;
            led_para[led_name].times = 0;  // 渐变时间要缩短
        }

        if ((st == LED_ON) || (st == FLED_ON)) {            // 其它状态的LED熄灭状态
            led_para[led_name].dutyfactor = FLAG_BLED | 0;  // 占空比起始值
        } else if ((st == LED_OFF) || (st == FLED_OFF)) {   // 其它状态的LED全亮状态
            led_para[led_name].dutyfactor = FLAG_BLED | BLED_PWM_WIDTH;
        }

        led_para[led_name].destination = FLAG_BLED | level;  // 设置目标亮度值
    }
}

void bled_on(led_name_t led_name, uint16_t gradient_ms) {
    bled_set(led_name, 0, gradient_ms);
}

void bled_off(led_name_t led_name, uint16_t gradient_ms) {
    bled_set(led_name, 0xFF, gradient_ms);
}

void bled_toggle(led_name_t led_name, uint16_t gradient_ms) {
    led_status_t st;
    if (led_name < DRV_LED_NAME_END) {
        st = led_status(led_name);
        if (st & LED_ON_MASK)
            bled_set(led_name, 0xFF, gradient_ms);
        else
            bled_set(led_name, 0, gradient_ms);
    }
}

void led_flash(led_name_t led_name, uint16_t time_ms, uint8_t counter) {
    if (led_name < DRV_LED_NAME_END) {
        if (time_ms < FLED_TIME_MIN) time_ms = FLED_TIME_MIN;
        if (time_ms > FLED_TIME_MAX) time_ms = FLED_TIME_MAX;

#ifdef LED_ADVANCED
        led_para[led_name].counter = FLAG_FLED | 0;                             // 设置计数器
        led_para[led_name].dutyfactor = FLAG_FLED | (time_ms >> 1);             // 设置闪烁频率 (占空比50%)
        led_para[led_name].destination = FLAG_FLED | ((uint16_t)counter << 1);  // 设置闪烁次数
#endif

#if (LED_ON_LEVEL > 0)
        led_port_high(led_name);
#else
        led_port_low(led_name);
#endif
    }
}

// LED灯1ms扫描函数, LED高级功能必须要用此函数
void led_scan_1ms(void) {
    uint8_t i;
    uint16_t temp;

    for (i = 0; i < DRV_LED_NAME_END; i++) {
        if ((led_para[i].destination & FLAG_MASK) == FLAG_LED) {  // 普通灯
            continue;
        } else if ((led_para[i].destination & FLAG_MASK) == FLAG_BLED) {  // 呼吸灯
            if (++led_para[i].counter >= (FLAG_BLED | BLED_PWM_WIDTH)) {  // 新一轮PWM脉宽开始
                led_para[i].counter = FLAG_BLED | 0;
#if (LED_ON_LEVEL > 0)  // 关灯
                led_port_low((led_name_t)i);
#else
                led_port_high((led_name_t)i);
#endif

                temp = led_para[i].destination;
                if (led_para[i].times) {  // 渐变时间要拉长
                    if (++led_para[i].timei >= led_para[i].times) {
                        led_para[i].timei = 0;
                        if (led_para[i].dutyfactor < temp) {  // 占空比缓慢接近目标值
                            led_para[i].dutyfactor++;
                        } else if (led_para[i].dutyfactor > temp) {
                            led_para[i].dutyfactor--;
                        }
                    }
                } else {  // (led_para[i].times == 0) 渐变时间要缩短
                    if (led_para[i].dutyfactor < temp) {
                        led_para[i].dutyfactor += led_para[i].timei;  // 占空比快速接近目标值
                        if (led_para[i].dutyfactor > temp)
                            led_para[i].dutyfactor = temp;
                    } else if (led_para[i].dutyfactor > temp) {
                        led_para[i].dutyfactor -= led_para[i].timei;
                        if (led_para[i].dutyfactor < temp)
                            led_para[i].dutyfactor = temp;
                    }
                }
            }

            temp = led_para[i].dutyfactor;
            if (led_para[i].counter == temp) {  // 占空比处翻转LED灯
#if (LED_ON_LEVEL > 0)                          // 开灯
                led_port_high((led_name_t)i);
#else
                led_port_low((led_name_t)i);
#endif
            }
        } else if ((led_para[i].destination & FLAG_MASK) == FLAG_FLED) {  // 是闪烁灯
            temp = led_para[i].dutyfactor;
            if (++led_para[i].counter >= temp) {
                led_para[i].counter = FLAG_FLED | 0;
                led_port_toggle((led_name_t)i);

                if (led_para[i].destination > (FLAG_FLED | 0)) {
                    if (--led_para[i].destination <= (FLAG_FLED | 0)) {  // 闪烁次数到, 闪烁结束
                        led_para[(led_name_t)i].counter = FLAG_LED | 0;
                        led_para[(led_name_t)i].dutyfactor = FLAG_LED | 0;
                        led_para[(led_name_t)i].destination = FLAG_LED | 0;
#if (LED_ON_LEVEL > 0)  // 关灯
                        led_port_low((led_name_t)i);
#else
                        led_port_high((led_name_t)i);
#endif
                    }
                }
            }
        }
    }
}

#endif /* LED_ADVANCED */

#ifdef DRV_GPIO_TEST
/*
 * 测试指令: led_test [init/on/off/toggle/flash/status] [0/1/2/ <DRV_LED_NAME_END]
 */
void led_test(char argc, char *argv) {
    uint8_t i;
    char fun[TEST_ARGV_LEN_MAX] = "init", para[TEST_ARGV_LEN_MAX] = "0";  // 设定默认值
    led_name_t name;

    for (i = 1; i < argc; i++) {  // 提取指令
        if (i == 1)
            snprintf(fun, TEST_ARGV_LEN_MAX, &(argv[(uint8_t)argv[i]]));
        else if (i == 2)
            snprintf(para, TEST_ARGV_LEN_MAX, &(argv[(uint8_t)argv[i]]));
        else
            break;
    }

    name = atoi(para);
    if (!strcmp("init", fun)) {
        led_init_all();
#ifndef LED_ADVANCED
    } else if (!strcmp("on", fun)) {
        led_on(name);
    } else if (!strcmp("off", fun)) {
        led_off(name);
    } else if (!strcmp("toggle", fun)) {
        led_toggle(name);
#else
    } else if (!strcmp("on", fun)) {
        led_on(name);
    } else if (!strcmp("off", fun)) {
        led_off(name);
    } else if (!strcmp("toggle", fun)) {
        bled_toggle(name, 0);
    } else if (!strcmp("flash", fun)) {
        led_flash(name, 500, 5);
#endif /* LED_ADVANCED */
    } else if (!strcmp("status", fun)) {
        LOG("led status: 0x%x, level: 0x%lx\r\n", led_status(name), led_port_level(name));
    } else {
        LOG("led cmd err\r\n");
    }
}
#else
__INLINE void led_test(char argc, char *argv) {}
#endif /* DRV_GPIO_TEST */

#endif /* DRV_LED_NAME_GPIO */
