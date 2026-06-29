# Implementation Plan: MAX30102 SpO2 Algorithm Metrics

## Checklist

1. Load pre-development Trellis specs before editing code.
2. Inspect `14_spo2_sensor` build files to confirm how new source files are included.
3. Add `spo2_processor.h/.c` with:
   - fixed-size 5 s ring buffer for 100 Hz red/IR samples
   - warmup/sample-count tracking
   - DC and AC estimation
   - ratio-of-ratios and SpO2 fixed-point conversion
   - pulse peak detection and HR estimation
   - quality/status classification
4. Update `main.c` to:
   - feed each raw sample into the processor
   - print a metric CSV header
   - print periodic `SPO2_METRIC` lines after processor updates
   - keep or gate raw `SPO2` debug lines so validation remains possible
5. Update the sample build file if the new processor source is not automatically included.
6. Build or run the closest available static/build check.
7. Provide manual verification steps for the HH-D01 board and debug UART.

## Validation

- Run `git diff --check`.
- Build the WS63 sample with `ENABLE_MYDEMO_SAMPLE` and `SAMPLE_SUPPORT_SPO2_SENSOR` selected if the local build environment supports it.
- Flash/run on HH-D01 and verify:
  - boot banner still reports MAX30102 ready
  - raw red/IR changes with finger contact
  - metric lines show `WARMUP` first, then either `OK` or a clear quality reason
  - `spo2_x10` is percent-like, for example around `900..1000` when signal quality is good
  - `hr_bpm` is plausible and invalid when no stable pulse is detected

## Risk Points

- User's observed raw samples around red `1300` and ir `1600` may be low for stable SpO2. The initial algorithm may report `LOW_SIGNAL` until sensor placement, LED current, or ADC range is tuned.
- The PDF does not provide the vendor lookup table, so the baseline conversion is the AN6409 linear approximation, isolated for future calibration.
- Motion and ambient light can produce plausible-looking but wrong values; quality flags are required.
- Build environment may have existing CMake generator cache issues. Do not delete build directories unless the user explicitly approves.

## Review Gate

Do not run `task.py start` and do not edit implementation code until the user approves this task summary.

After implementation, do not archive the task and do not commit code until the user manually verifies the board output and explicitly commands archive/commit.
