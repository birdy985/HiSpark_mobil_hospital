#include <stdint.h>

#include "ad8232_bsp.h"
#include "app_init.h"
#include "common_def.h"
#include "soc_osal.h"
#include <stdio.h>

#define AD8232_TASK_PRIO       26
#define AD8232_TASK_STACK_SIZE 0x1000
#define AD8232_SAMPLE_MS       10

static void *ad8232_task(const char *arg)
{
    unused(arg);

    errcode_t ret = ad8232_init();
    if (ret != ERRCODE_SUCC) {
        return NULL;
    }

    uint32_t seq = 0;
    while (1) {
        uint16_t voltage_mv = 0;
        ret = ad8232_read_mv(&voltage_mv);
        if (ret == ERRCODE_SUCC) {
            printf("%u,%u\r\n", (unsigned int)seq++, (unsigned int)voltage_mv);
        } else {
            printf("[AD8232] read failed, ret=0x%x\r\n", (unsigned int)ret);
        }
        osal_msleep(AD8232_SAMPLE_MS);
    }

    return NULL;
}

static void ad8232_entry(void)
{
    osal_task *task_handle = NULL;

    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)ad8232_task, NULL, "Ad8232Task", AD8232_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, AD8232_TASK_PRIO);
    } else {
        printf("[AD8232] create task failed\r\n");
    }
    osal_kthread_unlock();
}

app_run(ad8232_entry);
