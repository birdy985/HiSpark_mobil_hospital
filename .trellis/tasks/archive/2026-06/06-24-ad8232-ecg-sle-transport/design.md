# Design: AD8232 ECG Samples over SLE

## Architecture

Implement an ECG-over-SLE sample based on the existing AD8232 ADC sample and the existing SLE UART sample.

Preferred shape:

- Keep `9_ad8232_adc` as the standalone serial ADC/ECG sample.
- Add a new `mydemo` sample directory for ECG SLE transport, likely `13_ad8232_sle` or a similarly clear name.
- Reuse/copy the minimum SLE server/client setup pattern from `12_sle`, then replace UART forwarding with ECG packet send/receive.
- Reuse the existing AD8232 modules from `9_ad8232_adc` where practical:
  - `ad8232_bsp`
  - `ecg_processor`
  - `ecg_monitor_sample_t`

This keeps the old ADC sample available and avoids coupling the ECG acquisition path to the generic UART passthrough demo.

## Roles

### ECG SLE Client

The client board owns AD8232 acquisition:

1. Initialize AD8232 ADC.
2. Initialize ECG processor.
3. Initialize SLE client and connect/discover the server characteristic.
4. In an acquisition task, read ADC millivolts every `AD8232_SAMPLE_MS`.
5. Build `ecg_input_sample_t`.
6. Call `ecg_processor_process(&input, &output)`.
7. Packetize the resulting `ecg_monitor_sample_t output`.
8. Send via SSAP write request after the SLE link and handles are ready.

If SLE is not ready, the first implementation should record a drop counter and avoid blocking the ADC loop indefinitely.

### ECG SLE Server

The server board owns SLE advertising and receive-side parsing:

1. Initialize SLE server and advertise an ECG service/characteristic.
2. Accept client connection and characteristic writes.
3. Copy incoming payloads into a small queue or validate them immediately only if the callback remains short and bounded.
4. Validate packet magic, version, type, length, and checksum.
5. Parse ECG fields into a local packet/sample representation.
6. Detect sequence gaps per connected client.
7. Print stable CSV logs for plotting and diagnostics.

## Packet Contract

Use a packed, fixed-width binary packet for SLE payloads. Avoid sending C struct memory directly unless the wire struct is separately defined with explicit field widths and packing.

Planned fields:

```c
typedef struct {
    uint16_t magic;          /* ECG_SLE_PACKET_MAGIC */
    uint8_t version;         /* ECG_SLE_PACKET_VERSION */
    uint8_t type;            /* sample packet */
    uint16_t payload_len;    /* bytes after header before checksum */
    uint32_t seq;
    uint32_t timestamp_ms;
    uint16_t voltage_mv;
    int16_t baseline_mv;
    int16_t ecg_mv;
    int16_t filtered_mv;
    int16_t display_mv;
    uint8_t r_peak;
    uint8_t quality;
    uint16_t rr_interval_ms;
    uint16_t bpm;
    uint16_t checksum;
} ecg_sle_sample_packet_t;
```

Notes:

- `quality` is serialized as `uint8_t` instead of transmitting enum storage directly.
- `payload_len` protects future versioning and malformed write handling.
- `checksum` can initially be a simple 16-bit additive checksum if no local CRC helper is already in use; search first before implementing.
- All multi-byte fields are little-endian by default on WS63-to-WS63. If a PC/other-endian receiver is added later, add explicit encode/decode helpers.

## SLE Service Contract

Define ECG-specific UUIDs in a header instead of reusing the generic SLE UART UUIDs unchanged.

Minimum service shape:

- One ECG service.
- One writable ECG sample characteristic from client to server.
- Optional notify/indicate characteristic for ACK/status is deferred unless link behavior requires it.

The first task does not require application-level ACK/retry, but it must detect gaps at the server. If loss/backpressure is observed, a later task can add ACK/retry or frame batching.

## Data Flow

```text
AD8232 OUT -> ADC2 -> ad8232_read_mv
  -> ecg_processor_process
  -> ecg_monitor_sample_t output
  -> ecg_sle_sample_packet_t
  -> SLE SSAP write
  -> server write callback / queue
  -> packet validate + parse
  -> gap detection
  -> CSV log
```

## Logging

Client:

- Startup and connection state logs.
- Error logs for ADC/SLE failures.
- Rate-limited counters for sent/dropped samples.
- No per-sample hot-path printf by default.

Server:

- Startup and connection state logs.
- CSV header once.
- One CSV line per valid sample for waveform plotting unless the user chooses summary logging.
- Explicit logs for malformed packets and sequence gaps.

Suggested server CSV:

```text
ECG_SLE,seq,t_ms,raw_mv,base_mv,ecg_mv,filt_mv,disp_mv,r,rr_ms,bpm,q
```

## Backpressure Policy

Initial policy:

- Do not block the ADC sampling task forever waiting for SLE.
- If the SLE link/handle is not ready or send fails, increment a drop counter.
- Log drop counters periodically, not per sample.

This preserves sampling cadence during early prototype testing. A lossless workflow would need ACK/retry or buffering and is out of this first implementation scope.

## Build Integration

Likely changes after approval:

- Add a new sample directory under `fbb_ws63/src/application/samples/mydemo/`.
- Add role options under `mydemo/Kconfig`, following `12_sle/Kconfig`.
- Add sample registration under `mydemo/CMakeLists.txt`.
- Include AD8232 sources and ECG SLE sources in the sample `CMakeLists.txt`.

Avoid modifying target/board/boot configuration.

## Risks and Trade-offs

- 100 Hz single-sample writes may be acceptable for a prototype but are inefficient. If packet loss or send pressure appears, batch multiple samples per SLE packet in a later task.
- Reusing the SLE UART sample pattern reduces API risk, but ECG-specific UUIDs and packet validation are required to avoid a hidden text passthrough design.
- Server full CSV output is useful for plotting but can become heavy during long tests.

## Rollback

Rollback should remove the new ECG SLE sample directory and its `mydemo/Kconfig` / `mydemo/CMakeLists.txt` registrations. Existing `9_ad8232_adc` and `12_sle` samples should remain intact.
