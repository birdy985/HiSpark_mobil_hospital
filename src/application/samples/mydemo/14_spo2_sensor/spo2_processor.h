#ifndef SPO2_PROCESSOR_H
#define SPO2_PROCESSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "max30102_bsp.h"

#define SPO2_PROCESSOR_SAMPLE_RATE_HZ       100U
#define SPO2_PROCESSOR_WINDOW_SECONDS       5U
#define SPO2_PROCESSOR_WINDOW_SIZE          (SPO2_PROCESSOR_SAMPLE_RATE_HZ * SPO2_PROCESSOR_WINDOW_SECONDS)
#define SPO2_PROCESSOR_REPORT_INTERVAL      SPO2_PROCESSOR_SAMPLE_RATE_HZ

#define SPO2_PROCESSOR_STATUS_WARMUP        "WARMUP"
#define SPO2_PROCESSOR_STATUS_OK            "OK"
#define SPO2_PROCESSOR_STATUS_NO_FINGER     "NO_FINGER"
#define SPO2_PROCESSOR_STATUS_LOW_SIGNAL    "LOW_SIGNAL"
#define SPO2_PROCESSOR_STATUS_SATURATED     "SATURATED"
#define SPO2_PROCESSOR_STATUS_NO_PULSE      "NO_PULSE"
#define SPO2_PROCESSOR_STATUS_UNSTABLE      "UNSTABLE"

typedef struct {
    uint32_t red_dc;
    uint32_t ir_dc;
    uint32_t red_ac;
    uint32_t ir_ac;
    uint32_t ratio_x1000;
    int32_t spo2_x10;
    uint32_t hr_bpm;
    bool spo2_valid;
    bool hr_valid;
    const char *quality;
} spo2_metric_t;

typedef struct {
    uint32_t red[SPO2_PROCESSOR_WINDOW_SIZE];
    uint32_t ir[SPO2_PROCESSOR_WINDOW_SIZE];
    uint16_t write_index;
    uint16_t sample_count;
    uint32_t total_samples;
    uint32_t last_report_sample;
    uint32_t last_valid_hr_bpm;
    uint8_t hr_candidate_count;
} spo2_processor_t;

void spo2_processor_init(spo2_processor_t *processor);
bool spo2_processor_add_sample(spo2_processor_t *processor, const max30102_sample_t *sample,
    spo2_metric_t *metric);

#endif
