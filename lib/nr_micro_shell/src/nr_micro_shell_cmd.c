/**
 * \file            nr_micro_shell_cmd.c
 * \brief           shell cmd source file
 */

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "ezos_cfg.h"
#include "nr_micro_shell.h"
#include "task.h"

/* shell cmd config: 定义指令和函数 */
#define SHELL_CMD_FUN_DESC                         \
    X("help", cmd_help)                            \
    X("test", cmd_test, "test func")               \
    X("add", cmd_add, "add task [para]")           \
    X("force", cmd_force, "force add task [para]") \
    X("del", cmd_del, "del task")                  \
    X("sort", cmd_sort, "sort mem")

/* test cmd config: 定义测试指令(test)支持的函数 */
#define TEST_CMD_FUN_DESC                                               \
    X("led", led_test, "led [init/on/off/toggle/flash/status] [0/1/n]") \
    X("ctrl", ctrl_test, "ctrl [init/on/off/toggle/status] [0/1/n]")    \
    X("key", key_test, "key [init/scan]")

/* task cmd config: 定义任务指令(add/force/del)支持的函数 */
#define TASK_CMD_NAME_DESC               \
    X("led", TASK_LED)                   \
    X("hello", TASK_HELLO)               \
    X("hsm", TASK_HSM)                   \
    X("ipc", TASK_IPC, "send sem or mq") \
    X("sem", TASK_WAIT_SEM, "wait sem")  \
    X("mq", TASK_WAIT_MQ, "wait mq")     \
    X("mem0", TASK_MEM0)                 \
    X("mem1", TASK_MEM1)                 \
    X("mem2", TASK_MEM2)                 \
    X("malloc", TASK_MALLOC)             \
    X("show", TASK_SHOW, "show mem")

/* 设置任务参数的保存模式, 建议使用默认值 1. */
/* 0-所有任务共享同一个静态变量; 1-使用独立的静态变量进行参数传递; 2-使用mem申请独立内存 */
#define TASK_PARA_MODE 1
#define TASK_PARA_SIZE_MAX 16  // 设置静态变量缓冲大小

/* declaration */
#define X(cmd, fun, ...) void fun(char argc, char *argv);  // 生成指令函数
SHELL_CMD_FUN_DESC
#undef X

#define X(cmd, fun, ...) extern void fun(char argc, char *argv);  // 外部测试函数
TEST_CMD_FUN_DESC
#undef X

/* variable */

// 终端指令列表
const static_cmd_st static_cmd[] = {
#define X(cmd, fun, ...) {cmd, fun, ##__VA_ARGS__},
    SHELL_CMD_FUN_DESC
#undef X
    {"", NULL},
};

// (test) 命令可用的测试指令列表
typedef static_cmd_st static_test_st;

const static_test_st test_cmd_list[] = {
#define X(cmd, fun, ...) {cmd, fun, ##__VA_ARGS__},
    TEST_CMD_FUN_DESC
#undef X
    {"", NULL},
};

// (add/force/del)命令可用的任务指令列表
typedef enum {
    CMD_ADD,
    CMD_FORCE,
    CMD_DEL,
} action_t;

typedef struct {
    char cmd[NR_SHELL_CMD_NAME_MAX_LENGTH];
    task_name_t name;
    char *description;
} static_task_st;

const static_task_st task_cmd_list[] = {
#define X(cmd, name, ...) {cmd, name, ##__VA_ARGS__},
    TASK_CMD_NAME_DESC
#undef X
    {"", EZOS_TASK_IDLE},
};

#if (TASK_PARA_MODE == 1)
typedef struct {
    char buff[TASK_PARA_SIZE_MAX];
} static_para_st;

static_para_st task_para[] = {
#define X(...) {{0}},
    TASK_CMD_NAME_DESC
#undef X
};
#endif /* TASK_PARA_MODE */

/* function */
void cmd_help(char argc, char *argv) {
    uint16_t i, j;
    for (i = 0; static_cmd[i].fp != NULL; i++) {
        shell_printf("%s", static_cmd[i].cmd);
        if (static_cmd[i].description) shell_printf(" \t: %s", static_cmd[i].description);
        shell_printf("\r\n");
        if (!strcmp("test", static_cmd[i].cmd)) {
            for (j = 0; test_cmd_list[j].fp != NULL; j++) {
                shell_printf("  %s", test_cmd_list[j].cmd);
                if (test_cmd_list[j].description) shell_printf(" \t: %s", test_cmd_list[j].description);
                shell_printf("\r\n");
            }
        } else if (!strcmp("del", static_cmd[i].cmd)) {
            for (j = 0; task_cmd_list[j].name < EZOS_TASK_IDLE; j++) {
                shell_printf("  %s", task_cmd_list[j].cmd);
                if (task_cmd_list[j].description) shell_printf(" \t: %s", task_cmd_list[j].description);
                shell_printf("\r\n");
            }
        }
    }
}

void cmd_test(char argc, char *argv) {
    uint16_t i;
    if (argc > 1) {
        for (i = 0; test_cmd_list[i].fp != NULL; i++) {
            if (!strcmp(test_cmd_list[i].cmd, &argv[(uint8_t)argv[1]])) {
                memcpy(argv, argv + 1, argc - 1);     // 去掉test指令, 重写 argv 内容
                test_cmd_list[i].fp(argc - 1, argv);  // 去掉第一个参数: test
                return;
            }
        }
    }

    shell_printf("unknow test cmd:\r\n");
    for (i = 0; i < argc; i++) {
        shell_printf("  paras %02d: %s\r\n", i, &(argv[(uint8_t)argv[i]]));
    }
}

void cmd_task(char argc, char *argv, action_t action) {
    uint16_t i;
    ez_err_t rtn = 0xFF;
    if (argc > 1) {
        for (i = 0; task_cmd_list[i].name < EZOS_TASK_IDLE; i++) {
            if (!strcmp(task_cmd_list[i].cmd, &argv[(uint8_t)argv[1]])) {
                if (action == CMD_DEL) {
                    ezos_delete(task_cmd_list[i].name);
                    rtn = EZOS_OK;
                } else if (argc == 2) {
                    if (action == CMD_ADD) {
                        rtn = task_add(task_cmd_list[i].name);
                    } else {
                        rtn = force_add(task_cmd_list[i].name);
                    }
                    return;
                } else {
                    char *pbuff = NULL;
#if (TASK_PARA_MODE == 1)
                    pbuff = task_para[i].buff;
#elif (TASK_PARA_MODE == 2)
                    pbuff = ezos_malloc(task_cmd_list[i].name, TASK_PARA_SIZE_MAX, NULL);
#else
                    static char static_buff[TASK_PARA_SIZE_MAX];
                    pbuff = static_buff;
#endif /* TASK_PARA_MODE */
                    if (pbuff) {
                        snprintf(pbuff, TASK_PARA_SIZE_MAX, &argv[(uint8_t)argv[2]]);
                        if (action == CMD_ADD) {
                            rtn = task_add_para(task_cmd_list[i].name, pbuff);
                        } else {
                            rtn = force_add_para(task_cmd_list[i].name, pbuff);
                        }
                    } else {
                        rtn = 0xF0;
                    }
                }
                break;
            }
        }
    }
    if (rtn == 0xFF) {
        shell_printf("unknow task cmd\r\n");
    } else if (rtn == 0xF0) {
        shell_printf("%s(%d) fail to malloc\r\n", task_cmd_list[i].cmd, task_cmd_list[i].name);
    } else if (rtn != EZOS_OK) {
        shell_printf("%s(%d) fail to add\r\n", task_cmd_list[i].cmd, task_cmd_list[i].name);
    }
}

void cmd_add(char argc, char *argv) {
    cmd_task(argc, argv, CMD_ADD);
}

void cmd_force(char argc, char *argv) {
    cmd_task(argc, argv, CMD_FORCE);
}

void cmd_del(char argc, char *argv) {
    cmd_task(argc, argv, CMD_DEL);
}

void cmd_sort(char argc, char *argv) {
    ezos_mem_sort();
}
