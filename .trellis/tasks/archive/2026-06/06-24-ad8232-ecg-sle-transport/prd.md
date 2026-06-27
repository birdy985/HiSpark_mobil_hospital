# Send AD8232 ECG samples over SLE

## Goal

Build a WS63/FBB SDK prototype that sends processed AD8232 ECG monitor samples from a SLE client node to a SLE server node.

The client must use the existing AD8232 ADC processing output (`ecg_monitor_sample_t output`) as the source data and transfer those fields over Star Flash / SLE. The server must provide matching service code that receives, validates, parses, and logs the ECG samples.

This task is a prototype transport feature for development and waveform verification, not a certified medical-device feature.

## Confirmed Evidence

- Existing AD8232 sample:
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/main.c`
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/ecg_processor.h`
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/ecg_processor.c`
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/ad8232_bsp.c`
- Existing AD8232 data contract:
  - `ecg_monitor_sample_t` contains `seq`, `timestamp_ms`, `voltage_mv`, `baseline_mv`, `ecg_mv`, `filtered_mv`, `display_mv`, `r_peak`, `rr_interval_ms`, `bpm`, and `quality`.
  - `main.c` already creates `ecg_monitor_sample_t output` after each ADC read and currently prints it as CSV.
  - Current sample interval is `AD8232_SAMPLE_MS = 10`, which is a nominal 100 Hz path.
- Existing SLE UART sample:
  - `fbb_ws63/src/application/samples/mydemo/12_sle/main.c`
  - `fbb_ws63/src/application/samples/mydemo/12_sle/sle_uart_server/`
  - `fbb_ws63/src/application/samples/mydemo/12_sle/sle_uart_client/`
  - The sample already has selectable server/client Kconfig options and SSAP write/notification flow.
- Current sample registration:
  - `fbb_ws63/src/application/samples/mydemo/Kconfig` has `SAMPLE_SUPPORT_AD8232_ADC` and `SAMPLE_SUPPORT_SLE`.
  - `fbb_ws63/src/application/samples/mydemo/CMakeLists.txt` registers `9_ad8232_adc` and `12_sle`.
- Project rules require:
  - Follow local SDK samples and docs before inventing APIs.
  - Keep SLE callbacks light and hand off parsing/I/O to worker tasks or queues.
  - Include sequence number, timestamp, payload length, and checksum/CRC for waveform packets.
  - Detect gaps at the receiver.
  - Keep raw waveform frames binary over SLE; convert to printable logs at the receiver boundary.

## Requirements

- Add a SLE ECG transport sample or modules under `fbb_ws63/src/application/samples/mydemo/`, scoped to the AD8232/SLE feature and required build registration.
- Implement two build-selectable roles:
  - ECG SLE client: reads AD8232 ADC, processes each sample through `ecg_processor_process`, and sends the resulting `ecg_monitor_sample_t output` fields to the server.
  - ECG SLE server: advertises/connects using the matching SLE/SSAP service, receives ECG packets, validates them, parses them back into ECG sample fields, and prints machine-readable sample logs.
- Define a stable ECG-over-SLE packet contract that includes:
  - magic/version/type
  - sequence number
  - timestamp
  - all `ecg_monitor_sample_t` field values
  - payload length
  - checksum or CRC
- Preserve the AD8232 processing output contract. Do not remove or silently reinterpret existing fields.
- Avoid printf-heavy telemetry in the 100 Hz client hot path. Client-side logging should be minimal and rate-limited.
- Server logs should be sufficient for waveform plotting and debugging, including `seq`, `timestamp_ms`, `voltage_mv`, `ecg_mv`, `filtered_mv`, `display_mv`, `r_peak`, `rr_interval_ms`, `bpm`, and `quality`.
- The server must detect sequence gaps and malformed packets and report them clearly.
- Use bounded buffers and check all relevant API return values.
- Keep callbacks lightweight: packet validation/parsing can occur in a worker task if callback payload work becomes nontrivial.
- Define reconnection behavior: server restarts advertising on disconnect; client restarts scanning/connection flow on disconnect or pairing failure, following the existing sample pattern.
- Keep boot, partition, NV, signing, board, and unrelated target configuration untouched.

## Out of Scope

- Cloud upload, Wi-Fi, MQTT, database storage, or a PC GUI.
- Multi-client gateway topology beyond one ECG client and one server.
- Medical certification, diagnosis, alarm logic, or clinical validation.
- Lead-off GPIO support for AD8232 `LO+` / `LO-`; this can be a later task.
- Changing AD8232 ADC pin wiring unless explicitly requested.

## Acceptance Criteria

- [ ] Planning artifacts are reviewed and approved before implementation starts.
- [ ] ECG SLE client and ECG SLE server roles can be selected independently through Kconfig/menuconfig or an equivalent existing sample selection pattern.
- [ ] Client sends every successfully processed `ecg_monitor_sample_t output` sample, or records/rate-limits any explicit drop policy if SLE backpressure occurs.
- [ ] Server receives ECG packets and prints parsed sample lines in a stable machine-readable format.
- [ ] Server rejects packets with invalid magic/version/length/checksum and logs an error without crashing.
- [ ] Server detects sequence gaps and reports expected vs received sequence numbers.
- [ ] SLE callbacks avoid long blocking work and unsafe buffer use.
- [ ] All new files are registered in the correct `CMakeLists.txt` and `Kconfig` files.
- [ ] The selected build command is run in the SDK environment, or the exact build blocker is reported.
- [ ] Existing standalone `9_ad8232_adc` behavior is preserved unless explicitly replaced by the new ECG-over-SLE sample.

## Open Questions

- Product decision before implementation: should the server print every 100 Hz sample, or should it print every sample only in CSV mode and otherwise use rate-limited summary logs?

Recommended answer: print every received ECG sample as CSV on the server because the immediate goal is verifying AD8232 waveform transport, while keeping client logs rate-limited. Trade-off: full server CSV is easier to plot but may increase serial log volume during long runs.
