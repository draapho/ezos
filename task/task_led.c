#include "task.h"

// 对于非常简单的任务, 可以使用这种宏定义的方式来实现.
ezsm_t task_led(ezsm_t s, void* p) {
    EZOS_BEGIN(s);
    led_init(LD2);
    EZOS_YIELD(2000);
    bled_on(LD2);
    EZOS_YIELD(2000);
    bled_off(LD2);
    EZOS_YIELD(2000);
    bled_set(LD2, 200);
    EZOS_YIELD(2000);
    bled_toggle(LD2);
    EZOS_YIELD(2000);
    led_flash(LD2, 1000, 5);
    EZOS_YIELD(EZTM_AWHILE);
    EZOS_YIELD_UNTIL(100, led_status(LD2) == LED_OFF);  // 等待闪烁结束
    EZOS_GOTO_BEGIN();                                      // 跳转到 EZOS_BEGIN, 形成死循环.
    EZOS_END();                                             // 必须和 EZOS_BEGIN 配对使用
}
