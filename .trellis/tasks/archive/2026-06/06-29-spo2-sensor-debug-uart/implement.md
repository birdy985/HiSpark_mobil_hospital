# Implementation Plan: Read SpO2 sensor over debug UART

## Checklist

1. Load coding guidelines with `trellis-before-dev` before editing source.
2. Inspect existing `mydemo/10_iic`, `mydemo/9_ad8232_adc`, and sample Kconfig/CMake patterns immediately before editing.
3. Add `fbb_ws63/src/application/samples/mydemo/14_spo2_sensor` with:
   - `CMakeLists.txt`
   - `main.c`
   - `max30102_bsp.c`
   - `max30102_bsp.h`
   - `max30102_reg.h`
4. Implement MAX30102 helpers:
   - I2C pin init
   - single register write/read
   - multi-byte FIFO read
   - reset and FIFO clear
   - part ID check
   - SpO2 mode configuration
   - FIFO availability calculation with pointer wrap handling
5. Implement `Spo2Task`:
   - initialize BSP
   - print CSV header and configuration banner
   - poll FIFO periodically
   - print red/IR samples with sequence and timestamp
   - print read/init errors without crashing
6. Add `SAMPLE_SUPPORT_SPO2_SENSOR` to `mydemo/Kconfig`.
7. Add conditional `add_subdirectory_if_exist(14_spo2_sensor)` to `mydemo/CMakeLists.txt`.
8. Validate by running available lightweight checks:
   - `git diff --check`
   - targeted source inspection
   - repository build command if discoverable and feasible in this environment

## Validation Notes

The ideal validation is a WS63 sample build with `ENABLE_MYDEMO_SAMPLE` and `SAMPLE_SUPPORT_SPO2_SENSOR` selected. If the local environment cannot build, document the exact blocker and provide the files changed.

## Risk Points

- Kconfig option interactions can accidentally include multiple app samples. Keep the new option opt-in and avoid changing defaults for existing options.
- I2C address format in this SDK examples uses 7-bit device addresses. Use `0x57`, not `0xAE` or `0xAF`.
- `printf` float formatting is unreliable in existing samples; avoid floats in serial output.
- FIFO data is 18-bit and must be masked with `0x03FFFF`.

## Follow-up Checks Before Start

- Confirm task status is still `planning` before `task.py start`.
- After `task.py start`, load `trellis-before-dev` before source edits.
