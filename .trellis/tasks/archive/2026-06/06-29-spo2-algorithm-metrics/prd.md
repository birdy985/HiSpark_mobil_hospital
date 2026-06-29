# Process MAX30102 SpO2 metrics

## Goal

Add an on-device processing layer for the existing HXDZ-30102/MAX30102 sample so the debug UART can report medically meaningful demo estimates instead of only raw red/IR PPG samples.

The output should include estimated blood oxygen saturation as a percentage, heart rate in BPM, and enough signal-quality diagnostics to tell whether a value is usable. This is a prototype/development-board measurement, not a certified medical device reading.

## Confirmed Facts

- The current `14_spo2_sensor` sample initializes the MAX30102, reads FIFO red/IR samples, and prints raw CSV lines as `SPO2,seq,t_ms,red,ir,status`.
- The current MAX30102 setup uses SpO2 mode, 100 Hz sample rate, 411 us pulse width / 18-bit ADC data, FIFO averaging, and default LED pulse amplitudes.
- The HXDZ-30102 V4.2 manual states the algorithm stores a time window of red/IR reflection samples, computes RED/IR DC and AC components, computes ratio `R`, then uses a lookup table to determine SpO2.
- The same manual states heart rate is determined from the time difference `T` between adjacent peaks of one LED's AC component: `BPM = 60 / T`.
- The manual's serial example exposes `red`, `ir`, `HR`, `HRvalid`, `SPO2`, and `SPO2valid`.
- The manual recommends the larger-memory STM32/MBED-style configuration of 100 Hz, 32-bit data, and a continuously stored 5 s window for more stable values than Arduino's 25 Hz / 4 s mode.
- Maxim AN6409 describes post-processing as filtering, peak-to-peak detection, normalization, and DSP. For SpO2 it computes `R = (AC_red / DC_red) / (AC_ir / DC_ir)` and gives the baseline approximation `SpO2 = 104 - 17R`.
- Both the module manual and Maxim application note make clear that useful SpO2 requires proper signal processing and calibration; uncalibrated module output cannot guarantee clinical accuracy.

## Requirements

- Preserve raw sample visibility for debugging, but add a distinct metric output line so raw data is not confused with calculated SpO2.
- Maintain a bounded 5 s processing window at 100 Hz where RAM allows, matching the HXDZ manual's more stable platform guidance.
- Compute red/IR DC and AC components from the window, compute ratio-of-ratios `R`, and estimate SpO2 using a documented baseline curve/formula.
- Compute heart rate from detected pulse peaks over the recent window.
- Emit validity/quality fields so the user can distinguish usable readings from no-finger, low-signal, saturated, unstable, or no-pulse states.
- Use fixed-size buffers and integer/fixed-point arithmetic where practical for embedded stability.
- Keep the code scoped to the existing `14_spo2_sensor` sample; do not add OLED, BLE, phone app, cloud upload, or certified medical-device claims.
- Document that readings are development/demo estimates and must not be treated as diagnostic medical results without calibration and validation.

## Acceptance Criteria

- [ ] After enough samples are collected, debug UART emits a metric CSV header and repeated metric lines containing at least SpO2 percentage, HR BPM, validity flags, and signal diagnostics.
- [ ] SpO2 output is a percent-like value derived from red/IR AC/DC ratio, not the raw FIFO number.
- [ ] HR output is derived from pulse peak interval and is marked invalid when reliable peaks are not detected.
- [ ] Low-quality conditions are surfaced with explicit status labels instead of printing plausible-looking numbers.
- [ ] Existing MAX30102 initialization and raw FIFO reading remain functional.
- [ ] The implementation builds or, if local build tooling is blocked, the exact build blocker and manual verification steps are documented.

## Notes

- User requested planning only first. Do not start implementation until the user explicitly approves.
- User requested no task archive and no code commit after implementation until manual verification is complete and they explicitly command archive/commit.
