/**
 * @file      nr_micro_shell_commands.c
 * @author    Nrush
 * @version   V0.1
 * @date      28 Oct 2019
 * *****************************************************************************
 * @attention
 *
 * MIT License
 *
 * Copyright (C) 2019 Nrush. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Includes ------------------------------------------------------------------*/

#include <ctype.h>
#include <string.h>

#include "nr_micro_shell.h"
#include "task.h"

const char task_str[EZOS_TASK_NAME_END][20] = {
#define X(name, task, str, ...) str,  // 提取任务字符串, 按优先级排列
    EZOS_TASKS_NAME_FUN_STR
#undef X
};

void shell_help_cmd(char argc, char *argv) {
    unsigned int i = 0;
    for (i = 0; nr_shell.static_cmd[i].fp != NULL; i++) {
        shell_printf("%s", nr_shell.static_cmd[i].cmd);
        shell_printf("\n");
    }
    shell_printf("add or del [task_name], support following [task_name]\n");
    for (i = 0; i < EZOS_TASK_NAME_END; i++) {
        if (strcmp("", task_str[i])) {
            shell_printf("  %s\n", task_str[i]);
        }
    }
}

void shell_add_cmd(char argc, char *argv) {
    unsigned int i = 0;
    if (argc > 1) {
        for (i = 1; i < EZOS_TASK_NAME_END; i++) {
            if (!strcmp(task_str[i], &argv[(uint8_t)argv[1]])) {
                if (argc > 2) {
                    static char buff[30];
                    snprintf(buff, sizeof(buff), &argv[(uint8_t)argv[2]]);
                    force_add((task_name_t)i, &buff);
                } else {
                    force_add((task_name_t)i, NULL);
                }
                return;
            }
        }
    }
    shell_printf("unknow task to add\n");
}

void shell_del_cmd(char argc, char *argv) {
    unsigned int i = 0;
    if (argc > 1) {
        for (i = 1; i < EZOS_TASK_NAME_END; i++) {
            if (!strcmp(task_str[i], &argv[(uint8_t)argv[1]])) {
                ezos_delete((task_name_t)i);
                return;
            }
        }
    }
    shell_printf("unknow task to del\n");
}

void shell_test_cmd(char argc, char *argv) {
    unsigned int i;
    shell_printf("test command:\n");
    for (i = 0; i < argc; i++) {
        shell_printf("paras %d: %s\n", i, &(argv[(uint8_t)argv[i]]));
    }
}

void shell_sort_cmd(char argc, char *argv) {
	printf("before mem sort\n");
	show_mem();
	ezos_mem_sort();
	printf("after mem sort\n");
	show_mem();
}

const static_cmd_st static_cmd[] =
    {
        {"help", shell_help_cmd},
        {"test", shell_test_cmd},
		{"led", led_test},
        {"sort", shell_sort_cmd},
        {"add", shell_add_cmd},
        {"del", shell_del_cmd},
        {"", NULL}};

/******************* (C) COPYRIGHT 2019 Nrush *****END OF FILE*****************/
