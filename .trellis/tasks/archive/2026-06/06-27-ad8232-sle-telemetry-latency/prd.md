# Reduce AD8232 SLE telemetry latency

## Goal

Reduce the apparent AD8232 SLE telemetry delay where the server continues printing old ECG samples while the client has already advanced far ahead. The fix should make the server process near-current samples for real-time waveform display and should keep enough diagnostics to compare client send-time data against server receive/decode data.

## Problem Evidence

- The client debug serial can print `ECG_SLE,seq,t_ms,...` immediately before `ecg_sle_client_send_sample(&output)`.
- The server debug serial prints the same CSV field order after `ecg_sle_decode_sample()` succeeds.
- A captured comparison showed roughly:
  - Server: `seq` around 642, `timestamp_ms` around 18068.
  - Client: `seq` around 1454, `timestamp_ms` around 34769.
- The sequence gap is about 812 samples. At a 10-20 ms sample cadence, that is many seconds of backlog. This points to queued old packets, not a packet field decoding mismatch.

## Confirmed Facts From Code

- The packet format already includes `seq` and `timestamp_ms` and preserves all ECG fields needed for comparison.
- `ecg_sle_packet.c` encodes and decodes the same field order, so this task must not change the binary packet contract.
- The current client sampling loop calls `ecg_sle_client_send_sample(&output)` every sample once the handle is available.
- `ecg_sle_client_send_sample()` immediately calls `ssapc_write_req()`; it does not wait for `write_cfm_cb` before allowing the next write.
- `sle_uart_client_sample_write_cfm_cb()` currently logs every write confirm but does not expose write completion state to `main.c`.
- The server write callback copies every received packet into an OSAL queue, and the server task consumes packets FIFO.
- Both client and server currently have hot-path per-sample CSV/debug printing, which can distort timing during diagnostics.

## Requirements

- Keep the existing `ECG_SLE_PACKET_LEN`, field order, checksum, and encode/decode behavior unchanged.
- Keep the current AD8232 processing/filtering behavior unchanged.
- Make the client SLE send path backpressure-aware so software cannot build an unbounded multi-second backlog.
- Prefer real-time freshness over lossless replay for display/debug: when the link cannot keep up, stale samples may be dropped and counted instead of queued indefinitely.
- Add or preserve diagnostics that allow comparing:
  - client send-attempt sample `seq` and `timestamp_ms`
  - server receive/decode sample `seq` and `timestamp_ms`
  - server local receive/processing time, so transport/queue delay can be measured directly
- Reduce high-frequency logs that are not needed for latency comparison, especially per-write confirmation spam.
- Keep changes scoped to `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle` unless a build issue proves a minimal shared include change is required.
- Do not touch unrelated samples, board config, flash/partition/NV/signing config, or unrelated untracked files.

## Acceptance Criteria

- [ ] Client no longer calls `ssapc_write_req()` repeatedly while a previous ECG write is still pending confirmation.
- [ ] Client reports skipped/dropped samples due to SLE backpressure in periodic stats.
- [ ] Server logs include enough timing data to compute delay between decoded packet timestamp and server-side receive/process time.
- [ ] Per-write-confirm debug output is removed or throttled so it does not flood the debug serial during ECG streaming.
- [ ] Server does not spend time replaying many seconds of old queued samples under normal streaming conditions.
- [ ] Client and server CSV comparison can be done by matching `seq` values.
- [ ] `python build.py -c ws63-liteos-app -component=samples` passes for the default client configuration.
- [ ] A temporary server configuration build also passes before final completion.

## Out Of Scope

- Changing the ECG filter algorithm.
- Changing the AD8232 ADC sampling code.
- Redesigning the SLE binary packet format or adding multi-sample packet batching in the first fix.
- Cloud upload, persistence, or UI redesign.
- Making the transport medically lossless. This task prioritizes real-time display and correctness diagnostics.

## Open Decision For User

- Whether the first implementation should prioritize freshness by dropping samples whenever the previous SLE write is still pending. Recommended: yes, because ECG live display should not replay 10+ seconds of stale data.