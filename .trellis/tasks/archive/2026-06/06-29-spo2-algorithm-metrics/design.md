# Design: MAX30102 SpO2 Algorithm Metrics

## Scope

Extend the existing `14_spo2_sensor` sample with an algorithm module that consumes MAX30102 red/IR samples and produces demo SpO2, heart-rate, and signal-quality metrics over debug UART.

The design follows the HXDZ-30102 manual and Maxim AN6409 as the baseline:

1. Collect a recent red/IR PPG window.
2. Estimate DC and AC components for red and IR.
3. Compute ratio-of-ratios `R = (AC_red / DC_red) / (AC_ir / DC_ir)`.
4. Estimate SpO2 from `R`, initially with the documented linear approximation `SpO2 = 104 - 17R`.
5. Detect pulse peaks and compute HR from peak-to-peak interval.

## Architecture

- `max30102_bsp.c/.h` remains responsible only for sensor register access and raw FIFO reads.
- Add a small processing module, likely `spo2_processor.c/.h`, under `14_spo2_sensor`.
- `main.c` keeps the sampling loop, feeds each sample into the processor, and prints metric lines when the processor has a fresh estimate.
- Raw CSV output may remain available for debugging, but calculated values must use a different prefix/header to avoid implying raw values are SpO2.

## Data Flow

1. FIFO read returns one `max30102_sample_t` with 18-bit `red` and `ir`.
2. The sample loop pushes samples into a fixed-size ring buffer.
3. Once the 5 s / 100 Hz target window is warm, the processor computes:
   - `red_dc`, `ir_dc`: mean or baseline estimate for the window.
   - `red_ac`, `ir_ac`: pulse amplitude estimate after DC removal.
   - `ratio_x1000`: fixed-point ratio-of-ratios.
   - `spo2_x10`: estimated SpO2 in tenths of a percent.
   - `hr_bpm`: heart rate from accepted peak intervals.
   - `quality`: status label and/or bitmask.
4. The UART line emits both human-usable values and diagnostics.

Proposed metric line:

```text
SPO2_METRIC,seq,t_ms,spo2_x10,hr_bpm,ratio_x1000,red_dc,ir_dc,red_ac,ir_ac,quality
```

Example: `spo2_x10=968` means `96.8%`.

## Quality Model

The processor should prefer invalid output over misleading output. Suggested quality labels:

- `WARMUP`: not enough samples in the window yet.
- `OK`: SpO2 and HR are both currently plausible and stable.
- `NO_FINGER`: DC level or amplitude suggests no usable contact.
- `LOW_SIGNAL`: AC amplitude is too small for reliable ratio/peak detection.
- `SATURATED`: raw ADC samples are near the top of the 18-bit range.
- `NO_PULSE`: no reliable peak interval found.
- `UNSTABLE`: values or peak intervals vary too much over the window.

The exact numeric thresholds can be conservative initially and adjusted from captured serial logs.

## Algorithm Trade-Offs

- The vendor manual says production examples use a lookup table, but the table is not exposed in the PDF. AN6409 provides a transparent baseline formula, so the first implementation should use `SpO2 = 104 - 17R` and keep the conversion isolated so it can later be replaced by a calibrated table.
- A full clinical algorithm would require calibration against reference equipment, motion rejection, skin/contact compensation, and validation across users. This task should provide meaningful engineering estimates with quality flags, not medical certification.
- A 5 s window improves stability but delays the first valid output. This matches the manual's preferred STM32/MBED-style guidance.
- Fixed-point math avoids floating-point runtime assumptions and makes serial diagnostics deterministic.

## Compatibility

- Keep the existing I2C pin mapping and MAX30102 register setup unless implementation evidence shows signal levels require LED/ADC tuning.
- Do not remove raw data support; it is needed for manual validation and threshold tuning.
- Avoid changing unrelated samples or shared build files except for adding the new processor source to the existing sample target if required.

## Rollback

Rollback should be limited to removing the new processor files and restoring `main.c` to raw-only output. The existing MAX30102 BSP should remain reusable.
