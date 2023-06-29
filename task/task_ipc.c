#include "task.h"

// 信号量范例.
// 终端内调用 "add ipc", 演示 sem 信号量;
// 终端内调用 "add ipc xxx", 演示 mq 信号量.
/*
终端测试1:
    add sem
    add ipc
终端测试2:
    add mq
    add ipc hello
终端测试3:
    add ipc
    add ipc
    add sem
    add sem
    add sem
    add ipc
终端测试4:
    add ipc str1
    add ipc str2
    add ipc str3
    add ipc str4
    add mq
    add mq
    add mq
    add mq
    add ipc str5
 */

ez_err_t ezos_sem_init(ez_sem_t *sem, uint8_t value);  // 初始化信号量
ez_err_t ezos_mq_init(ez_mq_t *message, uint16_t msg_size, void *pool, uint16_t pool_size);

ez_sem_t sem;

ez_mq_t mq;
char mq_pool[90];
typedef struct {
    char msg[30];
} mq_msg_t;

void ipc_test_init(void) {
    ezos_sem_init(&sem, 0);
    ezos_mq_init(&mq, sizeof(mq_msg_t), mq_pool, sizeof(mq_pool));  // 90 / 30 = 3, 可以缓冲3个msg.
}

ezsm_t task_ipc(ezsm_t s, void *p) {
    if (p == NULL) {
        printf("releas sem\r\n");
        ezos_sem_release(&sem);
    } else {
        printf("send mq: %s\r\n", (char *)p);
        ezos_mq_send(&mq, p);
    }
    return EZSM_DONE;
}

ezsm_t task_sem(ezsm_t s, void *p) {
    static uint8_t times;
    switch (s) {
        case EZSM_INIT:
            printf("wait sem\r\n");
            times = 0;
            ezos_delay_awhile();
            return ++s;
        case 1: {
            ez_err_t err;
            err = ezos_sem_take(&sem);  // 尝试获取信号量
            if (err == EZOS_OK) {       // 成功获取
                if (times == 0) {
                    printf("done, take sem immediately\r\n");
                } else if (times == 1) {
                    printf("done, take sem in 5 second\r\n");
                } else {
                    printf("done, take sem finally\r\n");
                }
                return EZSM_DONE;
            } else if (err == EZOS_FAIL) {  // 获取失败, 重新等待
                if (times == 0) {
                    times = 1;
                    printf("fail to take sem. now wait 5s\r\n");
                    ezos_delay(5000);  // 延时5s, 看是否能获取信号量
                } else {
                    times = 2;
                    printf("fail to take sem in 5s. now wait forever\r\n");
                    ezos_delay_forever();
                }
                return s;
            } else {  // EZOS_ERROR, 不应该到这里
                return s;
            }
        }
        default:
            return EZSM_DONE;
    }
}

ezsm_t task_mq(ezsm_t s, void *p) {
    switch (s) {
        case EZSM_INIT:
            printf("wait mq\r\n");
            ezos_delay_awhile();
            return ++s;
        case 1: {
            ez_err_t err;
            char msg_buf[30];

            err = ezos_mq_receive(&mq, msg_buf);
            if (err == EZOS_OK) {  // 成功获取
                printf("done, got mq: %s\r\n", msg_buf);
                return EZSM_DONE;
            } else {
                printf("fail to get mq immediately, wait forever\r\n");
                ezos_delay_forever();  // 永久等待消息队列
                return s;
            }
        }
        default:
            return EZSM_DONE;
    }
}
