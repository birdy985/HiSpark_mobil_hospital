# Design: Reduce AD8232 SLE Telemetry Latency

## Summary

The latency symptom is consistent with software backlog. The client produces ECG samples faster than the SLE write/confirm path and debug serial can drain, while the server consumes received packets FIFO. The first fix should add explicit write-confirm backpressure and stale-sample dropping, then make diagnostics measure real latency without flooding the debug port.

## Data Flow

Current flow:

1. Client samples AD8232 every `ECG_SLE_SAMPLE_MS`.
2. Client calls `ecg_processor_process()`.
3. Client prints diagnostic data.
4. Client calls `ecg_sle_client_send_sample()`.
5. `ecg_sle_client_send_sample()` encodes one sample and calls `ssapc_write_req()`.
6. SLE client callback later receives `write_cfm_cb`.
7. Server write callback receives packet bytes and copies one packet into an OSAL queue.
8. Server task decodes FIFO packets, sends TJC display updates, and prints CSV.

Planned flow:

1. Client still samples and processes at the same cadence.
2. Client checks a SLE ECG write-ready state before calling `ssapc_write_req()`.
3. If the previous ECG write is pending, the sample is not queued; the client increments a backpressure/drop counter.
4. If ready, client marks a write pending and sends the current sample.
5. `write_cfm_cb` clears the pending flag and updates confirmation counters/status.
6. Server continues decoding the same packet format.
7. Server logs packet timestamp plus server-local receive/process time for delay measurement.
8. Hot-path logs are reduced or throttled so they do not become the main bottleneck.

## Contracts

### Packet Contract

No change:

- `ECG_SLE_PACKET_MAGIC`
- `ECG_SLE_PACKET_VERSION`
- `ECG_SLE_PACKET_LEN`
- payload field order
- checksum behavior

### Client Backpressure Contract

Add a small SLE client state interface in `sle_uart_client`:

- `bool sle_uart_client_can_send(void);`
- `void sle_uart_client_mark_write_pending(void);`
- `uint32_t sle_uart_client_get_write_cfm_count(void);` or equivalent stats accessor if useful.

Expected behavior:

- Before `ssapc_write_req()`, client code checks handle validity and pending state.
- On successful `ssapc_write_req()`, pending becomes true.
- In `write_cfm_cb`, pending becomes false regardless of success/failure, and status counters are updated.
- If `ssapc_write_req()` returns failure immediately, pending is cleared so the next fresh sample can try again.

### Server Timing Diagnostic Contract

Server CSV should stay comparable with current data. Add server-local timing either by:

- adding a separate diagnostic log such as `ECG_SLE_RX,seq,packet_t_ms,rx_ms,delay_ms`, or
- extending the existing server CSV only if the user accepts a changed CSV schema.

Recommended: add a separate `ECG_SLE_RX` line or a throttled summary so the original `ECG_SLE` CSV remains easy to compare.

## Trade-offs

### Drop Stale Samples vs Preserve Every Sample

Recommended: drop stale unsent samples when a write is pending.

Why: live ECG display is harmed more by multi-second backlog than by occasional missing samples. The packet already has `seq`, so gaps are visible and countable.

Trade-off: not suitable for lossless medical recording. If lossless capture is needed later, use a separate storage or batched transport design.

### Per-sample CSV vs Throttled Diagnostics

Recommended: keep comparison diagnostics during testing, but make high-frequency logs optional or throttled.

Why: serial logging can dominate runtime and make latency measurements worse than the actual SLE link.

Trade-off: less raw visibility in every sample, but better measurement fidelity.

### Single-sample Packets vs Batched Packets

Recommended for first fix: keep single-sample packets.

Why: it avoids protocol changes and isolates the backlog root cause.

Future option: batch 5-10 samples per SLE packet if throughput still needs improvement after backpressure/log cleanup.

## Rollback

Rollback is straightforward:

- Restore the previous `main.c` client send behavior.
- Restore the previous `sle_uart_client.c/.h` callback behavior.
- No data migration or persistent state is involved.