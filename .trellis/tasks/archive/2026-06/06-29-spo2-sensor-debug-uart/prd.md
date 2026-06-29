# Read SpO2 sensor over debug UART

## Goal

Add a WS63 mydemo sample that reads the HXDZ-30102/MAX30102 blood oxygen sensor over I2C and prints readable sensor data through the board debug UART.

The immediate value is hardware bring-up: confirm that the sensor is powered, addressable, configured, and producing red/IR PPG samples that can be viewed from a serial terminal. The first implementation should favor reliable raw-data output over a polished medical algorithm.

## Requirements

- Use the local sensor documentation under `D:\HiSparkSDK\血氧传感器` as the source of hardware and register behavior.
- Target the HXDZ-30102 basic module by default because the user asked for the blood oxygen sensor generally and the basic module documentation confirms it is a MAX30102 I2C PPG module.
- Communicate with the sensor over I2C using the existing WS63 SDK APIs and the existing `mydemo/10_iic` style as implementation reference.
- Print output through the existing debug serial/log path using `printf`, consistent with `mydemo/9_ad8232_adc`.
- Add the sample into the `fbb_ws63/src/application/samples/mydemo` sample structure with a dedicated Kconfig option and CMake inclusion.
- Initialize the I2C pins/bus, verify the MAX30102 part ID, configure FIFO / SpO2 mode / LED pulse amplitudes, then periodically read FIFO red and IR samples.
- Output a stable CSV-like line suitable for serial plotting or capture, for example `SPO2,seq,t_ms,red,ir,status`.
- Report clear init/read errors over the debug UART so wiring, address, and bus failures are visible.
- Keep this task scoped to sensor bring-up and data transport. Do not add BLE, OLED display, PC tools, or a certified medical-grade SpO2 calculation in this task.

## Confirmed Facts

- The project currently has no active Trellis task. This task was created in planning state at `.trellis/tasks/06-29-spo2-sensor-debug-uart`.
- The working tree already has uncommitted changes before this task, including `fbb_ws63`, `HH-D01/`, and `血氧传感器/`; implementation must not overwrite unrelated user work.
- `HXDZ-30102使用说明书V4.2.pdf` says the HXDZ-30102 module uses MAX30102, exposes `VCC`, `GND`, `SCL`, `SDA`, `INT`, outputs PPG data over I2C, and can work with 3.3V or 5V logic/power through the module board.
- The same manual states the module's raw data can be used for heart-rate and SpO2 calculation, and that other development boards can output calculated or raw data over serial.
- `MAX30102.pdf` confirms communication is I2C-compatible, max SCL is 400 kHz, write/read addresses are `0xAE`/`0xAF` (7-bit address `0x57`), FIFO data is at register `0x07`, and the part ID register `0xFF` should read `0x15`.
- Existing sample `fbb_ws63/src/application/samples/mydemo/10_iic` initializes I2C pins using `CONFIG_I2C_SCL_MASTER_PIN`, `CONFIG_I2C_SDA_MASTER_PIN`, and `uapi_i2c_master_init`, then reads registers with `uapi_i2c_master_writeread`.
- Existing sample `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc` prints CSV-like sensor data using `printf` in a periodic task.

## Acceptance Criteria

- [ ] A new blood oxygen sensor sample can be selected from the existing mydemo configuration without removing existing samples.
- [ ] On boot, the debug UART prints an init banner and either `PART_ID=0x15` or a clear error message if the MAX30102 is not reachable.
- [ ] With the sensor connected, the debug UART periodically prints CSV-like red/IR FIFO samples with sequence and timestamp fields.
- [ ] I2C read/write failures are reported with error codes and do not crash the task.
- [ ] The implementation compiles in the existing WS63 sample build path, or any build-blocking environment issue is documented with the exact command/output.
- [ ] No BLE, OLED, unrelated AD8232 behavior, or vendor SDK code outside the necessary sample/Kconfig/CMake path is changed.

## Notes

- This task is planning-only until the user explicitly approves implementation.
- Before implementation starts, add `design.md` and `implement.md` because this touches sample structure, I2C driver usage, sensor register configuration, and build selection.
