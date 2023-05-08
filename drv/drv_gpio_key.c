/**
 * \file            drv_gpio_key.c
 * \brief           KEY driver source file. 仅支持单按键的判断
 */

#include "drv_gpio_key.h"

/* key porting */
static const gpio_hw_t key_hw[DRV_KEY_NAME_END] = {  // 按键硬件映射表
#define X(name, port, pin) {port, pin},
    DRV_KEY_NAME_GPIO
#undef X
};

// 按键 IO口初始化
__STATIC_INLINE void key_port_init(key_name_t key_name) {
    drv_input_init(&key_hw[key_name]);
}

// 获取按键 IO口当前电平值. 0, 低电平. !0, 高电平
__STATIC_INLINE keyint_t KEY_PORT_LEVEL(key_name_t key_name) {
    return drv_input_level(&key_hw[key_name]);
}

/* variable */
volatile key_event_t key_event = {0};  // 按键状态变量

/* function */
void key_init_all(void) {
    uint8_t i;
    ASSERT(DRV_KEY_NAME_END <= (sizeof(keyint_t) << 3));  // 定义的按键数量不得超过keyint_t的bit数量

    key_clear_all();
    for (i = 0; i < DRV_KEY_NAME_END; i++)
        key_init((key_name_t)i);
}

void key_init(key_name_t key_name) {
    ASSERT(key_name < (sizeof(keyint_t) << 3));  // 定义的按键数量不得超过keyint_t的bit数量
    ASSERT(key_name < DRV_KEY_NAME_END);
    if (key_name < DRV_KEY_NAME_END) {
        key_port_init(key_name);
    }
}

// 按键扫描函数, 时序要求不严格. 建议放在高优先级任务中.
void key_scan(void) {
    uint8_t bit, i;
    static uint8_t key_state = 0;
    static uint16_t temp_time;
    static keyint_t key_status_old = 0;

    key_event.on_off = 0;
    for (i = 0; i < DRV_KEY_NAME_END; i++) {
        bit = !KEY_PORT_LEVEL((key_name_t)i);  // 获取按键当前电平值
        key_event.on_off |= (bit << i);        // 提取当前按键状态
    }

    switch (key_state) {
        case 0:                                   // 无按键状态
            if (key_event.on_off) key_state = 1;  // 有按键, 到按键确认状态
            key_event.on_off = 0;                 // 不能让主程序认为按键已经按下
            key_status_old = 0;
            break;
        case 1:                      // 按键确认按下状态
            if (key_event.on_off) {  // 确认了是有按键
#if (KEY_SUPPORT & KEY_TOUCH)
                key_event.touch |= (key_event.on_off ^ key_status_old) & key_event.on_off;
#endif
                key_status_old = key_event.on_off;
                temp_time = 0;
                key_state = 2;
            } else {  // 是误触发
                key_state = 0;
            }
            break;
        case 2:                                                              // 短按 / 长按检测
            if (key_event.on_off == key_status_old) {                        // 按键没有改变
                if (++temp_time > (KEY_LONG_PRESS_MS / KEY_SCAN_TICK_MS)) {  // 进入长按状态
#if (KEY_SUPPORT & KEY_LCLICK)
                    key_event.lclick |= key_status_old;
#endif
#if (KEY_SUPPORT & KEY_LPRESS)
                    key_event.lpress |= key_status_old;
#endif
                    temp_time = 0;
                    key_state = 3;
                }
            } else {  // 按键释放, 判定为短按
#if (KEY_SUPPORT & KEY_CLICK)
                key_event.click |= (key_event.on_off ^ key_status_old) & ~key_event.on_off;
#endif
                key_state = 1;
            }
            break;
        case 3:
            if (key_event.on_off == key_status_old) {
                if (++temp_time > (KEY_RUN_PRESS_MS / KEY_SCAN_TICK_MS)) {  // 连发时间间隔
#if (KEY_SUPPORT & KEY_LPRESS)
                    key_event.lpress |= key_status_old;
#endif
                    temp_time = 0;
                }
            } else {
                key_state = 1;
            }
            break;
    }
}

#if 0  // key task example
void task_key_example(char argc, char *argv) {
    key_scan();               // 按键定时扫描

    // 方法一:
    switch (key_event.touch)  // 按键按下时的事件
    {
        case KEY_MASK(KEY_EXAMPLE):
            // dosomething();
            key_clear(&key_event.touch, KEY_EXAMPLE);  // 如有必要, 手动清空
            break;
    }    

    // 方法二:
    switch (key_clear(&key_event.touch, DRV_KEY_ALL))  // 读取事件, 并自动清空
    {
        case KEY_MASK(KEY_EXAMPLE): 
            // dosomething();
            break;
    }  

    // 方法三:
    if (key_clear(&key_event.lpress, KEY_EXAMPLE)) {    // 读取指定按键事件, 并自动清空
        // dosomething();
    }

    // 方法四:
    if (key_get(&key_event.lclick, KEY_EXAMPLE)) {
        // dosomething();
        key_clear(&key_event.lclick, KEY_EXAMPLE);  // 如有必要, 手动清空
    }

    key_clear_all(); // 最后清空全部事件
}
#endif

#ifdef DRV_GPIO_TEST
/*
 * 测试指令: key_test [init/scan]
 */
void key_test(char argc, char *argv) {
    uint8_t i;
    char fun[TEST_ARGV_LEN_MAX] = "init";  // 设定默认值

    for (i = 1; i < argc; i++) {  // 提取指令
        if (i == 1)
            snprintf(fun, TEST_ARGV_LEN_MAX, &(argv[(uint8_t)argv[i]]));
    }

    if (!strcmp("init", fun)) {
        key_init_all();
    } else if (!strcmp("scan", fun)) {
        LOG("start key_scan, dead loop!\r\n");
        while (1) {      // 进入死循环, 将无法响应其它指令!
            key_scan();  // 按键定时扫描
            for (key_name_t i = 0; i < DRV_KEY_NAME_END; i++) {
#if (KEY_SUPPORT & KEY_TOUCH)
                if (key_clear(&key_event.touch, i))
                    LOG("key %d touched\r\n", i);
#endif
#if (KEY_SUPPORT & KEY_CLICK)
                if (key_clear(&key_event.click, i))
                    LOG("key %d clicked\r\n", i);
#endif
#if (KEY_SUPPORT & KEY_LCLICK)
                if (key_clear(&key_event.lclick, i))
                    LOG("key %d long clicked\r\n", i);
#endif
#if (KEY_SUPPORT & KEY_LPRESS)
                if (key_clear(&key_event.lpress, i))
                    LOG("key %d long pressed\r\n", i);
#endif
            }
            delay_us(1000 * 10);  // 延时10ms
        }
    }
}
#else
__INLINE void key_test(char argc, char *argv) {}
#endif /* DRV_GPIO_TEST */
