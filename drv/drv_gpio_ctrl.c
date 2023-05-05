/**
 * \file            drv_gpio_ctrl.c
 * \brief           Control port driver source file.
 */

#include "drv_gpio_ctrl.h"

/* ctrl porting */
typedef struct {
    gpio_hw_t io;
    uint8_t level;
} ctrl_hw_t;

static const ctrl_hw_t ctrl_hw[DRV_CTRL_NAME_END] = {  // 控制端口硬件映射表
#define X(name, port, pin, on) {{port, pin}, on},
    DRV_CTRL_NAME_PORT_PIN_ON
#undef X
};

// 控制端口初始化
__STATIC_INLINE void ctrl_port_init(ctrl_name_t ctrl_name) {
    drv_output_init(&ctrl_hw[ctrl_name].io);
}

// 控制端口电平置高
__STATIC_INLINE void ctrl_port_high(ctrl_name_t ctrl_name) {
    drv_output_high(&ctrl_hw[ctrl_name].io);
}

// 控制端口电平置低
__STATIC_INLINE void ctrl_port_low(ctrl_name_t ctrl_name) {
    drv_output_low(&ctrl_hw[ctrl_name].io);
}

// 控制端口口电平翻转
__STATIC_INLINE void ctrl_port_toggle(ctrl_name_t ctrl_name) {
    drv_output_toggle(&ctrl_hw[ctrl_name].io);
}

// 读取控制端口电平值
__STATIC_INLINE uint32_t ctrl_port_level(ctrl_name_t ctrl_name) {
    return drv_output_level(&ctrl_hw[ctrl_name].io);
}

/* function */
void ctrl_init_all(void) {
    for (uint8_t i = 0; i < DRV_CTRL_NAME_END; i++)
        ctrl_init((ctrl_name_t)(i));
}

void ctrl_init(ctrl_name_t ctrl_name) {
    if (ctrl_name < DRV_CTRL_NAME_END) {
        ctrl_port_init(ctrl_name);
        ctrl_off(ctrl_name);
    }
}

void ctrl_on(ctrl_name_t ctrl_name) {
    if (ctrl_name < DRV_CTRL_NAME_END) {
        if (ctrl_hw[ctrl_name].level)
            ctrl_port_high(ctrl_name);
        else
            ctrl_port_low(ctrl_name);
    }
}

void ctrl_off(ctrl_name_t ctrl_name) {
    if (ctrl_name < DRV_CTRL_NAME_END) {
        if (ctrl_hw[ctrl_name].level)
            ctrl_port_low(ctrl_name);
        else
            ctrl_port_high(ctrl_name);
    }
}

void ctrl_toggle(ctrl_name_t ctrl_name) {
    if (ctrl_name < DRV_CTRL_NAME_END) {
        ctrl_port_toggle(ctrl_name);
    }
}

uint8_t ctrl_status(ctrl_name_t ctrl_name) {
    if (ctrl_name >= DRV_CTRL_NAME_END) return 0;

    if (ctrl_hw[ctrl_name].level)
        return (ctrl_port_level(ctrl_name) != 0);
    else
        return (ctrl_port_level(ctrl_name) == 0);
}

#ifdef DRV_GPIO_TEST
/*
 * 测试指令: ctrl_test [init/on/off/toggle/status] [0/1/2/ <DRV_CTRL_NAME_END]
 */
void ctrl_test(char argc, char *argv) {
    char fun[TEST_ARGV_LEN_MAX] = "init", para[TEST_ARGV_LEN_MAX] = "0";  // 设定默认值
    ctrl_name_t name;

    for (uint8_t i = 1; i < argc; i++) {  // 提取指令
        if (i == 1)
            snprintf(fun, TEST_ARGV_LEN_MAX, &(argv[(uint8_t)argv[i]]));
        else if (i == 2)
            snprintf(para, TEST_ARGV_LEN_MAX, &(argv[(uint8_t)argv[i]]));
        else
            break;
    }

    name = atoi(para);
    if (!strcmp("init", fun)) {
        ctrl_init_all();
    } else if (!strcmp("on", fun)) {
        ctrl_on(name);
    } else if (!strcmp("off", fun)) {
        ctrl_off(name);
    } else if (!strcmp("toggle", fun)) {
        ctrl_toggle(name);
    } else if (!strcmp("status", fun)) {
        LOG("ctrl status: %d, level: 0x%lx\n", ctrl_status(name), ctrl_port_level(name));
    } else {
        LOG("ctrl cmd err\n");
    }
}
#else
__INLINE void ctrl_test(char argc, char *argv) {}
#endif /* DRV_GPIO_TEST */
