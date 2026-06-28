# Implementation Plan: Reduce AD8232 SLE Telemetry Latency

## Pre-checks

- Confirm current dirty state before editing. There is already an uncommitted client diagnostic CSV change in `13_ad8232_sle/main.c`; preserve or intentionally fold it into this task only after user approval.
- Do not stage or modify unrelated files such as `串口屏协议.png` or `血氧传感器/`.

## Files Expected To Change

- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/main.c`
- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/sle_uart_client/sle_uart_client.c`
- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/sle_uart_client/sle_uart_client.h`

Possible but not planned unless needed:

- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/sle_uart_server/sle_uart_server.c`
- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/sle_uart_server/sle_uart_server.h`

## Step-by-step Checklist

1. Add client write-pending state in `sle_uart_client.c`.
   - Static pending flag.
   - Write confirm counter.
   - Last write status if useful.
   - Clear pending in `sle_uart_client_sample_write_cfm_cb()`.

2. Expose minimal accessors in `sle_uart_client.h`.
   - `sle_uart_client_can_send()` or equivalent.
   - `sle_uart_client_mark_write_pending()` if pending is set outside `sle_uart_client.c`.
   - Keep API small and specific to the sample.

3. Update `ecg_sle_client_send_sample()` in `main.c`.
   - Check handle and pending state before encode/write.
   - Return a distinguishable failure or existing `ERRCODE_FAIL` when skipped by backpressure.
   - Mark pending only after `ssapc_write_req()` returns success.
   - Clear pending immediately if `ssapc_write_req()` returns failure.

4. Update client stats.
   - Add a counter for backpressure skipped samples.
   - Include it in the existing 2-second stats print.
   - Keep sent/drop/fail meaning clear.

5. Reduce write-confirm log flood.
   - Remove per-confirm `osal_printk`, or throttle it to periodic summary.
   - Preserve error logging for failed confirmations.

6. Add server-side timing diagnostic.
   - Capture `rx_ms = (uint32_t)uapi_tcxo_get_ms()` after decode or before decode handling.
   - Compute `delay_ms = rx_ms - sample.timestamp_ms` if both boards have comparable uptime only after synchronized start; otherwise print both values and compare by sequence.
   - Prefer a separate diagnostic line to avoid breaking existing `ECG_SLE` CSV parsing.

7. Keep or gate client CSV diagnostics.
   - For validation, keep the client pre-send `ECG_SLE` CSV or rename it to `ECG_SLE_TX` if the user wants easier filtering.
   - Avoid simultaneously enabling pure `display_mv` plot output and verbose CSV on the same serial when measuring latency.

8. Validate build.
   - Run `python build.py -c ws63-liteos-app -component=samples` from `fbb_ws63/src` for default client config.
   - Temporarily switch config to server and run the same build, then restore config.

9. Manual validation.
   - Flash client and server.
   - Open both debug serials at the same time.
   - Match `seq` values across client and server logs.
   - Confirm server `seq` follows client closely instead of lagging hundreds of samples.
   - Confirm skipped/backpressure count may increase under load, but backlog does not grow without bound.

## Success Signal

- Server sequence is near client sequence during steady streaming, preferably within a small number of samples rather than hundreds.
- If SLE cannot keep up, client reports skipped samples instead of queueing stale packets for delayed replay.
- Debug logs remain usable without write-confirm spam overwhelming the serial output.

## Rollback Point

Before editing, capture current diff. If behavior worsens, revert only the changes in the three expected files listed above.