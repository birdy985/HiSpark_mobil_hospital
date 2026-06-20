# Implementation Plan

Do not execute this plan until the user approves it.

## Preconditions

- [ ] User approves `prd.md`, `design.md`, and this implementation plan.
- [ ] AD8232 PDF/manual key points are confirmed with a usable PDF reader/tool or manual inspection:
  - output bias / mid-supply behavior
  - 3.3 V output range safety
  - `LO+` / `LO-` behavior
  - `SDN` / `ISDN` behavior
  - relevant module usage notes

## Ordered Steps

1. Re-read required Trellis specs before editing:
   - `.trellis/spec/backend/index.md`
   - `.trellis/spec/backend/ws63-sdk-source-map.md`
   - `.trellis/spec/backend/ws63-sdk-coding-standard.md`
   - `.trellis/spec/guides/index.md`
2. Reconfirm current sample state:
   - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/main.c`
   - `ad8232_bsp.c/.h`
   - sample `CMakeLists.txt`
3. Add `ecg_processor.h`:
   - input/output structs
   - quality enum
   - init/process APIs
4. Add `ecg_processor.c`:
   - baseline tracking
   - relative ECG calculation
   - lightweight filtering
   - R-peak detection
   - RR/BPM calculation
   - quality classification
5. Register `ecg_processor.c` in `9_ad8232_adc/CMakeLists.txt`.
6. Update `main.c`:
   - timestamp near acquisition
   - call processor
   - output parseable ECG fields
   - keep ADC2 wiring log
7. Keep implementation sample-local; do not change board, boot, partition, NV, signing, or UART pin configuration.
8. Run source-level checks:
   - `rg -n "ecg_processor|ECG,|AD8232|ADC_CHANNEL_2" fbb_ws63/src/application/samples/mydemo/9_ad8232_adc`
9. Attempt SDK build:
   - preferred: `cd fbb_ws63/src && python3 build.py ws63-liteos-app`
   - Windows fallback if `python3` is unavailable: `python build.py ws63-liteos-app`
   - if CMake cache generator conflict appears, report exact blocker before deleting build outputs
10. Manual validation after flashing:
    - no sensor / known voltage checks raw ADC behavior
    - electrodes attached: `ecg_mv`/`disp_mv` move around zero
    - clear beats produce intermittent `r=1`
    - BPM remains 0 until credible RR interval
    - poor contact reports `NO_R_PEAK`, `FLATLINE`, `NOISY`, or `SATURATED`

## Risk Points

- AD8232 PDF/manual details are not yet machine-read in this environment.
- Serial `printf` can reduce actual sampling rate and distort plotted shape.
- 100 Hz can estimate BPM but is limited for ECG morphology; 250 Hz is preferable if output bandwidth supports it.
- R-peak thresholds must be tuned with real electrode data.
- Electrode contact, power noise, and ADC spikes can dominate the waveform.

## Review Gate

Before running `task.py start`, present the plan to the user and wait for explicit approval.
