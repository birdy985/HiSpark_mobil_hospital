# Design: Read SpO2 sensor over debug UART

## Architecture

Add a new self-contained sample under `fbb_ws63/src/application/samples/mydemo/14_spo2_sensor`.

The sample will follow the existing mydemo style:

- `main.c` owns the OS task and serial output loop.
- `max30102_bsp.c/.h` owns I2C pin/bus initialization and MAX30102 register/FIFO access.
- `max30102_reg.h` owns register addresses and configuration constants.
- `CMakeLists.txt` contributes the sample source files to the parent `SOURCES` variable.
- `mydemo/Kconfig` and `mydemo/CMakeLists.txt` add a dedicated `SAMPLE_SUPPORT_SPO2_SENSOR` option and conditional inclusion.

## Data Flow

1. `app_run()` creates a `Spo2Task` thread.
2. The task initializes I2C pins using the same SDK macros used by `mydemo/10_iic`:
   - `CONFIG_I2C_SCL_MASTER_PIN`
   - `CONFIG_I2C_SDA_MASTER_PIN`
   - `CONFIG_I2C_MASTER_PIN_MODE`
   - `CONFIG_I2C_MASTER_BUS_ID`
3. The BSP initializes the I2C master at 400 kHz.
4. The BSP reads `PART_ID` from register `0xFF` and requires `0x15` for MAX30102.
5. The BSP resets the sensor, clears FIFO pointers, configures FIFO averaging/rollover, enables SpO2 mode, configures ADC/sample/pulse width, and sets LED pulse amplitudes.
6. The task polls FIFO write/read pointers, reads available red/IR samples from `FIFO_DATA`, and prints CSV-like records:
   `SPO2,seq,t_ms,red,ir,status`

## Contracts

- I2C address: MAX30102 7-bit address `0x57` derived from documented write/read addresses `0xAE`/`0xAF`.
- Part ID: register `0xFF` must read `0x15`.
- FIFO frame: SpO2 mode produces red and IR samples. Each sample is 18-bit data carried in three bytes. One red/IR pair is six FIFO bytes.
- UART/debug output: use `printf`, matching `mydemo/9_ad8232_adc`.

## Compatibility

- The new sample is opt-in through Kconfig and should not alter existing AD8232, IIC, WiFi, SLE, or vendor SDK behavior.
- Existing dirty worktree changes must be preserved.
- The sample should work as a bring-up tool even without the INT pin wired by polling FIFO state.

## Trade-offs

- This design outputs raw red/IR PPG data first. Heart-rate and SpO2 calculation is intentionally out of scope because stable raw sampling is the necessary hardware validation step and medical-grade algorithms require calibration and validation.
- Polling FIFO is simpler and more robust for initial bring-up than using the INT pin, and avoids requiring users to wire INT.
- Default LED current and sample configuration will be conservative and easy to tune in constants.

## Rollback

Rollback is limited to deleting `14_spo2_sensor` and removing its Kconfig/CMake entries from `mydemo`.
