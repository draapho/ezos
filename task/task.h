/**
 * \file            task.h
 * \brief           task header file.
 */

#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "drv_gpio_led.h"
#include "drv_gpio_key.h"
#include "drv_gpio_ctrl.h"
#include "drv_uart.h"
#include "ezos.h"
#include "main.h"
#include "nr_micro_shell.h"

void ipc_test_init(void);
void show_mem(void);

#ifdef __cplusplus
}
#endif

#endif
