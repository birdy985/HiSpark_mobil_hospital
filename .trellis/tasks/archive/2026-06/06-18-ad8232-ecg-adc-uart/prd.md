# Drive AD8232 ECG sensor over ADC UART

## Goal

Port the AD8232 ECG acquisition demo from the uploaded STM32/Keil reference project to the WS63 NearLink SDK so the development board can read the AD8232 analog ECG output through an ADC channel and send readable sample values over serial output.

This is a prototype data-acquisition task, not a medical-device validation task.

## Confirmed Evidence

- Uploaded AD8232 reference project:
  - `AD8232心电传感器/采集上传V2/采集上传V2/USER/main.c`
  - `AD8232心电传感器/采集上传V2/采集上传V2/HARDWARE/ADC/adc.c`
- STM32 reference behavior:
  - initializes ADC
  - reads ADC channel 0 in a loop
  - prints the raw ADC value with `printf("%d\r\n", value)`
  - delays 10 ms between samples
  - initializes USART2 at 9600 baud
- WS63 ADC official/local sources inspected:
  - `fbb_ws63/docs/zh-CN/software/设备驱动 开发指南/设备驱动 开发指南.md`
  - `fbb_ws63/src/include/driver/adc.h`
  - `fbb_ws63/src/drivers/chips/ws63/porting/adc/adc_porting.h`
  - `fbb_ws63/src/drivers/chips/ws63/porting/adc/adc_porting.c`
  - `fbb_ws63/src/application/samples/peripheral/adc/adc_demo_inc.c`
  - `fbb_ws63/src/application/samples/mydemo/8_adc/`
- WS63 ADC API shape:
  - `uapi_adc_init(...)`
  - `uapi_adc_power_en(...)`
  - `uapi_adc_open_channel(...)`
  - `adc_port_read(channel, &voltage)`
  - `uapi_adc_deinit()`
- `adc_port_read` returns a calibrated millivolt value in the local samples.
- Official ADC documentation says ADC input reference range is 0-3.3 V and six ADC input ports are available.
- Board pin evidence from `fbb_ws63/vendor/Kaihong_KHD-3863B/doc/README.md`:
  - Header pin 24: `IO_7` / `ADC_INPUT0`
  - Header pin 23: `IO_8` / `ADC_INPUT1`
  - Header pin 22: `IO_9` / `ADC_INPUT2`
  - Header pin 21: `IO_10` / `ADC_INPUT3`
  - Header pin 18: `IO_12` / `ADC_INPUT5`
  - Header pins 1, 2, 10, 20, 25: GND
  - Header pins 13, 14, 15: +5V
- User board is WS63 Runhe/HiHope `HH-D02`.
- User added HH-D02 documents under `HH-D02/`.
- HH-D02 official/local sources inspected:
  - `HH-D02/HH-D02 星闪开发板规格说明书-20241212-V03.pdf`
  - `HH-D02/HH-D02开发板原理图.pdf`
- HH-D02 specification confirms:
  - the board has 6 ADC inputs
  - board input power is typically 5 V
  - the board has an AMS1117 regulator converting 5 V to 3.3 V
- HH-D02 schematic page 1 confirms:
  - connector `J6` is `3861_16P_2.54MM`
  - `J6-2` is `3V3`
  - `J6-7` is `GPIO_09`
  - `J6-16` is `GND`
  - connector `J5` has `J5-2` GND and `J5-3` USB_5V
  - connector `J5-5` is `GPIO_01`, `J5-6` is `GPIO_11`, `J5-7` is `GPIO_07`, `J5-8` is `GPIO_08`, `J5-9` is `GPIO_03`, `J5-10` is `GPIO_02`, `J5-11` is `GPIO_04`, `J5-12` is `GPIO_00`
- WS63/SDK evidence maps ADC channel 2 to `GPIO_09` / `ADC2`; implementation will use `ADC_CHANNEL_2`.
- Important conflict:
  - `IO_7` and `IO_8` are also UART2 RX/TX alternatives.
  - Use `GPIO_09` / `ADC2` first for the AD8232 analog signal to avoid UART2 pin conflict.
- Current `8_adc` sample has a likely bug or mismatch:
  - `MQ2_ADC_CHANNEL` is `ADC_CHANNEL_2`, but `mq2_init()` opens `ADC_CHANNEL_0`.
  - The AD8232 implementation must open the same channel it reads.

## Requirements

- Add or adapt a WS63 sample that reads the AD8232 analog output using `ADC_CHANNEL_2` by default.
- Use HH-D02 `J6-7` / `GPIO_09` / `ADC2` as the initial ECG analog signal input.
- Send readings over the existing debug serial output using `printf` or `osal_printk` first, avoiding custom UART0/UART2 reconfiguration in the MVP.
- Print data in a simple machine-readable line format that can be monitored by a serial terminal.
- Include enough fields to validate behavior:
  - sequence number
  - measured millivolts
  - optional raw/timestamp field if the SDK API supports it without extra risk
- Use a worker task, not heavy work directly in `app_run`.
- Use bounded delays so the task never spins.
- Check ADC init/open/read return values and print error context.
- Keep implementation scoped to a sample/app directory and necessary sample build registration only.
- Document the exact wiring in the final result.

## Initial Wiring Plan

Recommended first wiring for the MVP:

- AD8232 `GND` -> HH-D02 `J6-16` GND. `J5-2` GND is also valid.
- AD8232 `3.3` -> HH-D02 `J6-2` 3V3.
- AD8232 `OUTPUT` / `OUT` -> HH-D02 `J6-7` / `GPIO_09` / `ADC2`.
- AD8232 `LO-` -> leave unconnected for the first ADC-only bring-up.
- AD8232 `LO+` -> leave unconnected for the first ADC-only bring-up.
- AD8232 `ISDN` / `SDN` -> leave unconnected if the module has an onboard pull-up and works normally; if output stays off/flat, tie it to 3.3 V to keep the AD8232 out of shutdown.
- Do not connect AD8232 `OUTPUT` to a WS63 ADC pin if the signal can exceed 3.3 V.
- Do not power this AD8232 module from HH-D02 5 V for the first test; use HH-D02 3.3 V so the AD8232 output stays compatible with WS63 ADC range.

Optional later wiring after MVP:

- AD8232 `LO+` and `LO-` lead-off pins can be connected to GPIO inputs to detect electrode disconnect state. These are digital lead-off indicators, not additional ECG analog channels. This is out of scope for the first ADC-only reading task.

## Acceptance Criteria

- [ ] Planning artifacts are reviewed and approved before implementation starts.
- [ ] The selected ADC channel and HH-D02 physical connector pins are documented.
- [ ] The implementation builds in the WS63 SDK environment, or the exact build blocker is reported.
- [ ] Firmware prints periodic ECG ADC measurements over the serial log.
- [ ] With AD8232 output disconnected or tied to GND, serial output is near 0 mV.
- [ ] With AD8232 output connected to a known safe voltage below 3.3 V, serial output changes accordingly.
- [ ] With AD8232 connected to electrodes, serial output changes over time rather than staying constant.
- [ ] No UART0/UART2 pin repurposing is done unless explicitly approved.
- [ ] ADC input over-voltage risk is called out in the final instructions.

## Open Questions

- Confirm whether the AD8232 module's `ISDN` pin is internally pulled up. If not, it must be tied to 3.3 V for normal operation.
