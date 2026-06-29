#include <stdint.h>
#include <stdio.h>

#include "app_init.h"
#include "common_def.h"
#include "max30102_bsp.h"
#include "soc_osal.h"
#include "spo2_processor.h"
#include "tcxo.h"

#define SPO2_TASK_PRIO               26
#define SPO2_TASK_STACK_SIZE         0x1800
#define SPO2_POLL_MS                 20
#define SPO2_EMPTY_FIFO_SLEEP_MS     50
#define SPO2_RAW_PRINT_INTERVAL      100

static void spo2_print_raw_sample(uint32_t seq, const max30102_sample_t *sample)
{
    printf("SPO2_RAW,%lu,%lu,%lu,%lu,OK\r\n",
        (unsigned long)seq,
        (unsigned long)uapi_tcxo_get_ms(),
        (unsigned long)sample->red,
        (unsigned long)sample->ir);
}

static void spo2_print_metric(uint32_t seq, const spo2_metric_t *metric)
{
    printf("SPO2_METRIC,%lu,%lu,%ld,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u,%s\r\n",
        (unsigned long)seq,
        (unsigned long)uapi_tcxo_get_ms(),
        (long)metric->spo2_x10,
        (unsigned long)metric->hr_bpm,
        (unsigned long)metric->ratio_x1000,
        (unsigned long)metric->red_dc,
        (unsigned long)metric->ir_dc,
        (unsigned long)metric->red_ac,
        (unsigned long)metric->ir_ac,
        metric->spo2_valid ? 1U : 0U,
        metric->hr_valid ? 1U : 0U,
        metric->quality);
}

static void *spo2_task(const char *arg)
{
    static spo2_processor_t processor;
    errcode_t ret;
    uint32_t seq = 0;

    unused(arg);

    ret = max30102_init();
    if (ret != ERRCODE_SUCC) {
        printf("[SPO2] init failed, ret=0x%x\r\n", (unsigned int)ret);
        return NULL;
    }

    spo2_processor_init(&processor);

    printf("[SPO2] csv raw: SPO2_RAW,seq,t_ms,red,ir,status\r\n");
    printf("[SPO2] csv metric: SPO2_METRIC,seq,t_ms,spo2_x10,hr_bpm,ratio_x1000,red_dc,ir_dc,red_ac,ir_ac,spo2_valid,hr_valid,quality\r\n");
    printf("[SPO2] SpO2 is a demo estimate from red/IR AC/DC ratio; not a calibrated medical result\r\n");

    while (1) {
        uint8_t available = 0;

        ret = max30102_get_fifo_sample_count(&available);
        if (ret != ERRCODE_SUCC) {
            printf("[SPO2] fifo count failed, ret=0x%x\r\n", (unsigned int)ret);
            osal_msleep(SPO2_EMPTY_FIFO_SLEEP_MS);
            continue;
        }

        if (available == 0) {
            osal_msleep(SPO2_POLL_MS);
            continue;
        }

        while (available > 0) {
            max30102_sample_t sample = {0};
            spo2_metric_t metric;

            ret = max30102_read_fifo_sample(&sample);
            if (ret != ERRCODE_SUCC) {
                printf("[SPO2] fifo read failed, ret=0x%x\r\n", (unsigned int)ret);
                break;
            }

            if ((seq % SPO2_RAW_PRINT_INTERVAL) == 0) {
                spo2_print_raw_sample(seq, &sample);
            }
            if (spo2_processor_add_sample(&processor, &sample, &metric)) {
                spo2_print_metric(seq, &metric);
            }
            seq++;
            available--;
        }

        osal_msleep(SPO2_POLL_MS);
    }

    return NULL;
}

static void spo2_entry(void)
{
    osal_task *task_handle = NULL;

    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)spo2_task, NULL, "Spo2Task", SPO2_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, SPO2_TASK_PRIO);
    } else {
        printf("[SPO2] create task failed\r\n");
    }
    osal_kthread_unlock();
}

app_run(spo2_entry);
