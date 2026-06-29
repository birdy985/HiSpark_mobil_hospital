#include "spo2_processor.h"

#include <stddef.h>

#include "max30102_reg.h"

#define SPO2_NO_FINGER_DC_MIN          500U
#define SPO2_LOW_SIGNAL_AC_MIN         10U
#define SPO2_SATURATION_MARGIN         4096U
#define SPO2_PEAKS_MAX                 12U
#define SPO2_INTERVALS_MAX             (SPO2_PEAKS_MAX - 1U)
#define SPO2_MIN_PEAK_INTERVAL         45U
#define SPO2_MAX_PEAK_INTERVAL         200U
#define SPO2_MIN_SPO2_X10              700
#define SPO2_MAX_SPO2_X10              1000
#define SPO2_HR_THRESHOLD_DIV          4U
#define SPO2_HR_MAX_STEP_BPM           20U
#define SPO2_HR_CONFIRM_COUNT          2U

static uint16_t spo2_buffer_pos(const spo2_processor_t *processor, uint16_t chronological_index)
{
    uint16_t start = 0;

    if (processor->sample_count >= SPO2_PROCESSOR_WINDOW_SIZE) {
        start = processor->write_index;
    }

    return (uint16_t)((start + chronological_index) % SPO2_PROCESSOR_WINDOW_SIZE);
}

static void spo2_metric_set_defaults(spo2_metric_t *metric, const char *quality)
{
    metric->red_dc = 0;
    metric->ir_dc = 0;
    metric->red_ac = 0;
    metric->ir_ac = 0;
    metric->ratio_x1000 = 0;
    metric->spo2_x10 = 0;
    metric->hr_bpm = 0;
    metric->spo2_valid = false;
    metric->hr_valid = false;
    metric->quality = quality;
}

static void spo2_compute_dc_ac(const spo2_processor_t *processor, spo2_metric_t *metric,
    uint32_t *red_max_out, uint32_t *ir_max_out)
{
    uint64_t red_sum = 0;
    uint64_t ir_sum = 0;
    uint32_t red_min = UINT32_MAX;
    uint32_t red_max = 0;
    uint32_t ir_min = UINT32_MAX;
    uint32_t ir_max = 0;
    uint16_t i;

    for (i = 0; i < processor->sample_count; i++) {
        uint16_t pos = spo2_buffer_pos(processor, i);
        uint32_t red = processor->red[pos];
        uint32_t ir = processor->ir[pos];

        red_sum += red;
        ir_sum += ir;
        if (red < red_min) {
            red_min = red;
        }
        if (red > red_max) {
            red_max = red;
        }
        if (ir < ir_min) {
            ir_min = ir;
        }
        if (ir > ir_max) {
            ir_max = ir;
        }
    }

    metric->red_dc = (uint32_t)(red_sum / processor->sample_count);
    metric->ir_dc = (uint32_t)(ir_sum / processor->sample_count);
    metric->red_ac = red_max - red_min;
    metric->ir_ac = ir_max - ir_min;
    *red_max_out = red_max;
    *ir_max_out = ir_max;
}

static int32_t spo2_calculate_spo2_x10(uint32_t ratio_x1000)
{
    int32_t spo2_x10 = 1040 - (int32_t)(((17ULL * ratio_x1000) + 50ULL) / 100ULL);

    if (spo2_x10 < 0) {
        return 0;
    }
    if (spo2_x10 > SPO2_MAX_SPO2_X10) {
        return SPO2_MAX_SPO2_X10;
    }
    return spo2_x10;
}

static void spo2_compute_ratio(spo2_metric_t *metric)
{
    uint64_t numerator;
    uint64_t denominator;

    numerator = (uint64_t)metric->red_ac * (uint64_t)metric->ir_dc * 1000ULL;
    denominator = (uint64_t)metric->ir_ac * (uint64_t)metric->red_dc;
    if (denominator == 0) {
        metric->ratio_x1000 = 0;
        metric->spo2_x10 = 0;
        return;
    }

    metric->ratio_x1000 = (uint32_t)((numerator + (denominator / 2ULL)) / denominator);
    metric->spo2_x10 = spo2_calculate_spo2_x10(metric->ratio_x1000);
}

static bool spo2_is_candidate_peak(uint32_t prev, uint32_t curr, uint32_t next, uint32_t threshold)
{
    return curr > threshold && curr > prev && curr >= next;
}

static void spo2_sort_intervals(uint16_t *intervals, uint16_t interval_count)
{
    uint16_t i;

    for (i = 1; i < interval_count; i++) {
        uint16_t value = intervals[i];
        int16_t j = (int16_t)i - 1;

        while (j >= 0 && intervals[j] > value) {
            intervals[j + 1] = intervals[j];
            j--;
        }
        intervals[j + 1] = value;
    }
}

static bool spo2_collect_peak_intervals(const spo2_processor_t *processor, uint32_t threshold,
    uint32_t *hr_bpm, bool *unstable)
{
    uint16_t peaks[SPO2_PEAKS_MAX] = {0};
    uint16_t intervals[SPO2_INTERVALS_MAX] = {0};
    uint16_t peak_count = 0;
    uint16_t i;
    uint16_t interval_count = 0;

    for (i = 1; i + 1U < processor->sample_count; i++) {
        uint32_t prev = processor->ir[spo2_buffer_pos(processor, (uint16_t)(i - 1U))];
        uint32_t curr = processor->ir[spo2_buffer_pos(processor, i)];
        uint32_t next = processor->ir[spo2_buffer_pos(processor, (uint16_t)(i + 1U))];

        if (!spo2_is_candidate_peak(prev, curr, next, threshold)) {
            continue;
        }

        if (peak_count == 0 || (uint32_t)(i - peaks[peak_count - 1U]) >= SPO2_MIN_PEAK_INTERVAL) {
            if (peak_count < SPO2_PEAKS_MAX) {
                peaks[peak_count++] = i;
            }
        } else {
            uint32_t last = processor->ir[spo2_buffer_pos(processor, peaks[peak_count - 1U])];
            if (curr > last) {
                peaks[peak_count - 1U] = i;
            }
        }
    }

    if (peak_count < 3U) {
        return false;
    }

    for (i = 1; i < peak_count; i++) {
        uint32_t interval = peaks[i] - peaks[i - 1U];

        if (interval < SPO2_MIN_PEAK_INTERVAL || interval > SPO2_MAX_PEAK_INTERVAL) {
            continue;
        }
        if (interval_count < SPO2_INTERVALS_MAX) {
            intervals[interval_count++] = (uint16_t)interval;
        }
    }

    if (interval_count < 2U) {
        return false;
    }

    {
        uint16_t median_interval;
        uint32_t filtered_sum = 0;
        uint16_t filtered_count = 0;
        uint32_t tolerance;
        uint32_t avg_interval;

        spo2_sort_intervals(intervals, interval_count);
        median_interval = intervals[interval_count / 2U];
        tolerance = median_interval / 4U;
        if (tolerance < 8U) {
            tolerance = 8U;
        }

        for (i = 0; i < interval_count; i++) {
            uint32_t delta = (intervals[i] > median_interval) ?
                (intervals[i] - median_interval) : (median_interval - intervals[i]);
            if (delta <= tolerance) {
                filtered_sum += intervals[i];
                filtered_count++;
            }
        }

        if (filtered_count < 2U) {
            return false;
        }

        avg_interval = (filtered_sum + (filtered_count / 2U)) / filtered_count;
        if (avg_interval == 0U) {
            return false;
        }
        *hr_bpm = (SPO2_PROCESSOR_SAMPLE_RATE_HZ * 60U + (avg_interval / 2U)) / avg_interval;
        *unstable = filtered_count < interval_count;
    }

    return true;
}

static bool spo2_find_hr(const spo2_processor_t *processor, uint32_t ir_dc, uint32_t ir_ac,
    uint32_t *hr_bpm, bool *unstable)
{
    uint32_t threshold_delta = ir_ac / SPO2_HR_THRESHOLD_DIV;
    uint32_t high_threshold;

    *hr_bpm = 0;
    *unstable = false;

    if (threshold_delta < SPO2_LOW_SIGNAL_AC_MIN) {
        threshold_delta = SPO2_LOW_SIGNAL_AC_MIN;
    }

    high_threshold = ir_dc + threshold_delta;
    return spo2_collect_peak_intervals(processor, high_threshold, hr_bpm, unstable);
}

static bool spo2_accept_hr(spo2_processor_t *processor, uint32_t measured_hr, uint32_t *accepted_hr)
{
    uint32_t delta;

    if (processor->last_valid_hr_bpm == 0U) {
        processor->last_valid_hr_bpm = measured_hr;
        processor->hr_candidate_count = 0;
        *accepted_hr = measured_hr;
        return true;
    }

    delta = (measured_hr > processor->last_valid_hr_bpm) ?
        (measured_hr - processor->last_valid_hr_bpm) : (processor->last_valid_hr_bpm - measured_hr);
    if (delta <= SPO2_HR_MAX_STEP_BPM) {
        processor->last_valid_hr_bpm = ((processor->last_valid_hr_bpm * 3U) + measured_hr + 2U) / 4U;
        processor->hr_candidate_count = 0;
        *accepted_hr = processor->last_valid_hr_bpm;
        return true;
    }

    processor->hr_candidate_count++;
    if (processor->hr_candidate_count >= SPO2_HR_CONFIRM_COUNT) {
        processor->last_valid_hr_bpm = measured_hr;
        processor->hr_candidate_count = 0;
        *accepted_hr = measured_hr;
        return true;
    }

    *accepted_hr = processor->last_valid_hr_bpm;
    return false;
}

void spo2_processor_init(spo2_processor_t *processor)
{
    uint16_t i;

    if (processor == NULL) {
        return;
    }

    for (i = 0; i < SPO2_PROCESSOR_WINDOW_SIZE; i++) {
        processor->red[i] = 0;
        processor->ir[i] = 0;
    }
    processor->write_index = 0;
    processor->sample_count = 0;
    processor->total_samples = 0;
    processor->last_report_sample = 0;
    processor->last_valid_hr_bpm = 0;
    processor->hr_candidate_count = 0;
}

bool spo2_processor_add_sample(spo2_processor_t *processor, const max30102_sample_t *sample,
    spo2_metric_t *metric)
{
    uint32_t red_max = 0;
    uint32_t ir_max = 0;
    uint32_t measured_hr = 0;
    bool unstable = false;

    if (processor == NULL || sample == NULL || metric == NULL) {
        return false;
    }

    processor->red[processor->write_index] = sample->red;
    processor->ir[processor->write_index] = sample->ir;
    processor->write_index = (uint16_t)((processor->write_index + 1U) % SPO2_PROCESSOR_WINDOW_SIZE);
    if (processor->sample_count < SPO2_PROCESSOR_WINDOW_SIZE) {
        processor->sample_count++;
    }
    processor->total_samples++;

    if ((processor->total_samples - processor->last_report_sample) < SPO2_PROCESSOR_REPORT_INTERVAL) {
        return false;
    }
    processor->last_report_sample = processor->total_samples;

    if (processor->sample_count < SPO2_PROCESSOR_WINDOW_SIZE) {
        spo2_metric_set_defaults(metric, SPO2_PROCESSOR_STATUS_WARMUP);
        return true;
    }

    spo2_metric_set_defaults(metric, SPO2_PROCESSOR_STATUS_OK);
    spo2_compute_dc_ac(processor, metric, &red_max, &ir_max);

    if (red_max >= (MAX30102_SAMPLE_MASK - SPO2_SATURATION_MARGIN) ||
        ir_max >= (MAX30102_SAMPLE_MASK - SPO2_SATURATION_MARGIN)) {
        metric->quality = SPO2_PROCESSOR_STATUS_SATURATED;
        return true;
    }

    if (metric->red_dc < SPO2_NO_FINGER_DC_MIN || metric->ir_dc < SPO2_NO_FINGER_DC_MIN) {
        metric->quality = SPO2_PROCESSOR_STATUS_NO_FINGER;
        return true;
    }

    if (metric->red_ac < SPO2_LOW_SIGNAL_AC_MIN || metric->ir_ac < SPO2_LOW_SIGNAL_AC_MIN) {
        metric->quality = SPO2_PROCESSOR_STATUS_LOW_SIGNAL;
        return true;
    }

    spo2_compute_ratio(metric);
    if (metric->spo2_x10 >= SPO2_MIN_SPO2_X10 && metric->spo2_x10 <= SPO2_MAX_SPO2_X10) {
        metric->spo2_valid = true;
    }

    if (spo2_find_hr(processor, metric->ir_dc, metric->ir_ac, &measured_hr, &unstable)) {
        if (!spo2_accept_hr(processor, measured_hr, &metric->hr_bpm)) {
            metric->quality = SPO2_PROCESSOR_STATUS_UNSTABLE;
            return true;
        }
        metric->hr_valid = true;
    } else {
        metric->quality = SPO2_PROCESSOR_STATUS_NO_PULSE;
        return true;
    }

    if (unstable || !metric->spo2_valid) {
        metric->quality = SPO2_PROCESSOR_STATUS_UNSTABLE;
        return true;
    }

    metric->quality = SPO2_PROCESSOR_STATUS_OK;
    return true;
}
