# Design: AD8232 ECG ADC UART Sample

## Architecture

Create a small WS63 application sample based on the existing `mydemo/8_adc` and official `samples/peripheral/adc` patterns:

- `app_run(...)` entry creates one acquisition task.
- The task initializes ADC once, opens the configured AD8232 ADC channel, then loops.
- Each loop reads one millivolt sample with `adc_port_read(...)`.
- The task prints a compact serial line and sleeps for a fixed interval.

The MVP uses the existing SDK serial log path (`printf` or `osal_printk`) instead of configuring a separate UART peripheral. This avoids conflicts with UART0 burn/log/test use and with `IO_7`/`IO_8` UART2 alternate functions.

## ADC Channel and Pin

Default channel:

- SDK channel: `ADC_CHANNEL_2`
- WS63 signal: `ADC2`
- GPIO signal: `GPIO_09`
- HH-D02 physical connector: `J6-7`, confirmed from `HH-D02/HH-D02开发板原理图.pdf`.
- HH-D02 power pins for sensor: `J6-2` = `3V3`, `J6-16` = `GND`.

Reason:

- `IO_7` / `ADC_INPUT0` and `IO_8` / `ADC_INPUT1` overlap with UART2 RX/TX alternate functions.
- `GPIO_09` is already used by the local `8_adc` sample comment as the practical ADC2 pin.
- Using ADC2 keeps the first implementation away from likely serial pin conflicts.
- The earlier Kaihong-specific physical pin 22 must not be used for HH-D02.

## AD8232 Pin Roles

The AD8232 module has more pins than the one ADC signal because it exposes power, analog output, lead-off status, and shutdown control:

- `GND`: common ground with HH-D02.
- `3.3`: sensor power input.
- `OUTPUT`: the single analog ECG waveform output; this is the only pin connected to the WS63 ADC in the MVP.
- `LO-`: digital lead-off output indicating electrode disconnect state; optional GPIO input later.
- `LO+`: digital lead-off output indicating electrode disconnect state; optional GPIO input later.
- `ISDN` / `SDN`: shutdown control. Normal operation needs shutdown disabled, normally by leaving it pulled high or tying it to 3.3 V if the module has no pull-up.

## Data Format

Initial serial output format:

```text
AD8232 seq=<n> mv=<millivolts>
```

This is intentionally simple so a serial terminal, Python script, or plotting tool can parse it later.

## Sampling Rate

The STM32 reference delays 10 ms between reads, which is 100 Hz nominal output.

Initial WS63 plan:

- Start with 10 ms loop delay if build/runtime logs remain stable.
- If serial output is too heavy or jittery, increase to 20 ms or 50 ms for first board bring-up.

The first task is to prove electrical connection and ADC driver behavior, not to optimize medical waveform fidelity.

## Voltage Safety

WS63 ADC input must stay within 0-3.3 V according to the official ADC documentation. AD8232 output must be verified before connection if the module is powered by 5 V.

Safe bring-up sequence:

1. Power common ground first.
2. Verify AD8232 output range with a multimeter or oscilloscope if available.
3. Connect AD8232 `GND` to HH-D02 `J6-16` GND.
4. Connect AD8232 `3.3` to HH-D02 `J6-2` 3V3.
5. Connect AD8232 `OUT` to HH-D02 `J6-7` / `GPIO_09` / `ADC2` only when the output is confirmed <= 3.3 V.
6. Leave `LO+` and `LO-` unconnected for the first test.
7. Leave `ISDN` unconnected initially; if the sensor appears shut down, tie `ISDN` to `J6-2` 3V3.

## Build Scope

Preferred implementation location:

- Adapt or add a sample under `fbb_ws63/src/application/samples/mydemo/`.

Likely files:

- sample `main.c`
- sample sensor module `.c/.h`
- parent `CMakeLists.txt` / `Kconfig` only if a new sample directory is created

Avoid:

- boot, flash partition, NV, signing, or board config changes
- custom UART0/UART2 reconfiguration for the first version

## Known Issue to Avoid

Do not repeat the `8_adc` mismatch where the read channel is `ADC_CHANNEL_2` but initialization opens `ADC_CHANNEL_0`. The AD8232 sample must open and read the same channel.

## Rollback

Because this is planned as a sample-only change, rollback should be deleting the new sample directory and removing its parent `CMakeLists.txt`/`Kconfig` registration, if any.
