# Implementation Plan: AD8232 ECG Samples over SLE

## Preconditions

- User reviews and approves `prd.md` and `design.md`.
- Task is activated with `task.py start` before code implementation begins.
- Before editing code, load `trellis-before-dev` and re-read the relevant backend specs.

## Ordered Checklist

1. Inspect SLE details needed for implementation:
   - `fbb_ws63/src/application/samples/mydemo/12_sle/sle_uart_server/`
   - `fbb_ws63/src/application/samples/mydemo/12_sle/sle_uart_client/`
   - relevant SLE docs in `fbb_ws63/docs/zh-CN/software/软件开发指南/软件开发指南.md`
   - any existing checksum/CRC helper in SDK sources.
2. Create a new ECG SLE sample directory under `fbb_ws63/src/application/samples/mydemo/`.
3. Add ECG SLE packet header/source:
   - packet constants
   - explicit fixed-width wire struct
   - encode/decode helpers
   - checksum helper
   - validation return codes/log strings.
4. Add ECG SLE server role:
   - ECG service/characteristic UUIDs
   - SLE server init and advertising based on `12_sle`
   - receive callback with bounded copy/validation path
   - sequence gap detection
   - CSV output.
5. Add ECG SLE client role:
   - SLE client init/connect/discover based on `12_sle`
   - AD8232 init and ECG processing loop
   - packetize `ecg_monitor_sample_t output`
   - send over SSAP write only after link/handle readiness
   - rate-limited sent/drop/error counters.
6. Wire build configuration:
   - sample `CMakeLists.txt`
   - `mydemo/Kconfig` options for client/server roles
   - `mydemo/CMakeLists.txt` registration.
7. Preserve existing `9_ad8232_adc` behavior.
8. Run formatting/manual review for buffer sizes, stack sizes, callback workload, and return-value checks.
9. Build in the SDK environment if available.

## Validation Commands

Primary build command from project spec:

```bash
python3 build.py ws63-liteos-app
```

Clean build if needed:

```bash
python3 build.py -c ws63-liteos-app
```

Menuconfig path if role toggles need manual selection:

```bash
python3 build.py -c ws63-liteos-app menuconfig
```

Do not claim firmware validation unless the build actually runs in the configured SDK environment.

## Manual Hardware Validation

1. Flash server firmware to one WS63 board and confirm it advertises/starts the ECG SLE server.
2. Flash client firmware to another WS63 board with AD8232 connected as already planned:
   - AD8232 `GND` -> HH-D02 GND
   - AD8232 `3.3` -> HH-D02 3V3
   - AD8232 `OUT` -> HH-D02 `J6-7` / `GPIO_09` / `ADC2`
3. Open server serial log and confirm CSV header appears.
4. Confirm server receives increasing `seq` values.
5. Confirm `display_mv`/`ecg_mv` changes when the AD8232 signal changes.
6. Power-cycle or disconnect client and confirm server reports connection state without crashing.
7. Reconnect and confirm advertising/scanning recovers according to the implemented role behavior.

## Risky Files / Rollback Points

Likely touched files:

- `fbb_ws63/src/application/samples/mydemo/Kconfig`
- `fbb_ws63/src/application/samples/mydemo/CMakeLists.txt`
- new ECG SLE sample directory under `fbb_ws63/src/application/samples/mydemo/`

Rollback:

- Delete the new sample directory.
- Remove the corresponding `Kconfig` options.
- Remove the corresponding `CMakeLists.txt` registration.
- Leave `9_ad8232_adc` and `12_sle` unchanged.

## Review Gates

- Packet fields exactly cover `ecg_monitor_sample_t output`.
- No direct enum-size or raw struct-memory dependency in the public wire format.
- SLE callbacks do not perform unbounded work.
- Buffer length checks exist before copies and parsing.
- Send failures and queue/backpressure behavior are visible through counters.
- Server detects sequence gaps.
- Build registration follows adjacent `mydemo` samples.
