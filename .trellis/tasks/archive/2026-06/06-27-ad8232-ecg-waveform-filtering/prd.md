# Optimize AD8232 ECG waveform display

## Goal

Improve the software-side AD8232 ECG signal path so the serial-screen waveform is visibly cleaner, more regular, and useful for demonstration/debugging instead of the current noisy raw-looking trace.

## User Value

The current screen trace has too many spikes and baseline jumps to visually resemble an ECG waveform. The user wants the output to be closer to the official demo reference: periodic, smooth enough to inspect R peaks, and not dominated by high-frequency noise or display scaling artifacts.

## Confirmed Facts

- Current AD8232 ECG code lives under `fbb_ws63/src/application/samples/mydemo/`.
- `9_ad8232_adc/` owns reusable AD8232 ADC and ECG processing code: `ad8232_bsp.*`, `ecg_processor.*`.
- `13_ad8232_sle/` sends processed ECG samples over SLE and prints CSV on the server.
- The SLE client loop samples every `ECG_SLE_SAMPLE_MS = 10`, so the intended processing rate is 100 Hz.
- Current `ecg_processor.c` already does slow baseline IIR subtraction, simple low-pass IIR, display step limiting, R-peak detection, and basic quality states.
- Existing packet fields include `voltage_mv`, `baseline_mv`, `ecg_mv`, `filtered_mv`, `display_mv`, `r_peak`, `rr_interval_ms`, `bpm`, and `quality`.
- The screen is already bound to `display_mv`; previous work intentionally avoided adding a new `screen_y` field.
- New requirement: the AD8232 SLE client must also print one local validation copy of the screen-bound display value to the onboard CH340/debug serial output.

## Requirements

- Keep the existing SLE packet format and field order unchanged.
- Keep the serial screen consuming `display_mv`; do not require screen-side protocol changes.
- On the SLE client side, keep sending samples to the SLE server and also print a duplicate minimal validation value to the onboard CH340/debug serial output using `printf("%d", disp_mv)` semantics, where `disp_mv` is the final screen-bound display value.
- Improve `display_mv` so it is derived from a more stable filtered ECG signal, not directly from noisy baseline-subtracted input.
- Add software filtering suitable for a 100 Hz embedded ECG demo path:
  - stronger baseline stabilization / high-pass behavior
  - low-pass smoothing for high-frequency ADC/electrode noise
  - spike suppression or median/outlier rejection for isolated samples
  - display amplitude limiting that avoids single bad points stretching the waveform
- Preserve debug visibility by keeping raw `voltage_mv`, `ecg_mv`, and `filtered_mv` meaningful.
- Keep processing lightweight enough for the WS63 sample loop.
- Keep R-peak and BPM behavior at least as stable as the current implementation.
- Avoid medical-diagnosis claims; this is a demo-quality waveform cleanup task.

## Acceptance Criteria

- [ ] `13_ad8232_sle` still builds with the existing packet length and no new SLE fields.
- [ ] `display_mv` is generated from the cleaned ECG path and is visibly less spiky on the serial screen than the current trace.
- [ ] `voltage_mv`, `ecg_mv`, `filtered_mv`, `display_mv`, `r_peak`, `rr_interval_ms`, `bpm`, and `quality` remain present in server CSV output.
- [ ] The SLE client still sends the normal sample to the server and also emits the same final display value locally over CH340/debug serial in the minimal `printf("%d", disp_mv)` format.
- [ ] The ADC/ECG processing period remains 10 ms unless a code comment documents why it must change.
- [ ] The implementation has bounded integer math and clamps output to prevent overflow.
- [ ] No screen-side binding change is required.
- [ ] The final response includes hardware caveats if remaining noise may come from electrode contact, power, or lead placement.

## Out Of Scope

- Changing AD8232 wiring or adding LO+/LO- GPIO lead-off detection.
- Adding a new packet field or changing the serial screen protocol.
- Implementing a certified medical ECG algorithm.
- Reworking SLE connection, pairing, or transport behavior beyond what is needed to send the existing sample.
- Building a separate PC-side parser or plotting app for the CH340 output.

## Open Questions

- Should the first implementation prioritize maximum smoothness for visual display, or preserve sharper QRS/R-peak morphology for heart-rate detection?
