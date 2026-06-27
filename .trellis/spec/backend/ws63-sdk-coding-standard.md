# WS63 SDK Coding Standard

> Rules for future embedded C work in `fbb_ws63`.

---

## Core Principles

- Prefer official local documentation and existing SDK samples over new abstractions.
- Keep application changes scoped to the relevant sample/app directory plus the build registration files required by the SDK.
- Do not change boot, partition, NV, signing, board, or target configuration unless the task explicitly requires it.
- Search before editing any CMake, Kconfig, macro, component name, task name, UUID, handle, pin, UART bus, or build target.
- Treat every API return value as meaningful. Check errors and log enough context to diagnose failures.
- Avoid long work in interrupts and callbacks. Use queues or worker tasks for parsing, I/O forwarding, cloud publish, or waveform processing.

---

## Directory and Build Rules

For learning samples:

- Place small demos under `fbb_ws63/src/application/samples/<category>/<sample>/`.
- Follow nearby sample structure: `CMakeLists.txt`, optional `Kconfig`, `main.c` or `src/` + `inc/`.
- Register the sample in the parent `CMakeLists.txt` and `Kconfig` using the same pattern as adjacent samples.

For a standalone product-style app:

- Follow the official `新建APP` flow in `SDK开发环境搭建 用户指南`.
- Create `fbb_ws63/src/application/ws63/<app>/`.
- Copy/adapt `fbb_ws63/src/application/ws63/ws63_liteos_application/CMakeLists.txt`.
- Add the new app directory in `fbb_ws63/src/application/ws63/CMakeLists.txt`.
- Register the component in `fbb_ws63/src/build/config/target_config/ws63/config.py`.

Build validation:

- Normal app target: `python3 build.py ws63-liteos-app`
- Clean app build: `python3 build.py -c ws63-liteos-app`
- Component build during development: `python3 build.py -c ws63-liteos-app -component=<component>`
- Menuconfig: `python3 build.py -c ws63-liteos-app menuconfig`
- Output images are under `output/ws63/fwpkg/ws63-liteos-app/`.

Do not claim firmware validation unless the build command actually ran in the SDK environment.

---

## Application Entry and Task Rules

- Use `app_run(entry_function)` as the application entry pattern.
- Create work in tasks, not in `app_run` entry itself.
- Match existing task APIs:
  - CMSIS: `osThreadNew`, `osDelay`, `osMessageQueueNew`
  - OSAL: `osal_kthread_create`, `osal_kthread_set_priority`, `osal_msleep`, `osal_printk`
- Avoid mixing CMSIS and OSAL in one module unless an existing local sample does so for the same reason.
- Give every task an explicit stack size and priority based on existing samples.
- For high-rate medical waveform paths, use queues/ring buffers and avoid printing in the hot path.
- Keep large buffers static or heap-managed with explicit lifetime; avoid large local stack arrays in tasks and callbacks.
- For every long-running loop, include a delay, blocking wait, queue receive, or event wait. Do not spin.

---

## Driver Rules

- Configure pinmux before initializing a peripheral.
- Check the board documentation and `IO复用关系.md` before selecting pins.
- Deinitialize or close peripherals when a sample stops using them if the driver supports it.
- Do not use UART0 for custom data without an explicit reason; default SDK use shares it across burning, AT, testsuite, and logs.
- If UART receive callbacks may do real work, prefer enabling `CONFIG_UART_SUPPORT_RX_THREAD` through menuconfig rather than doing heavy work in interrupt context.
- Initialize DMA before using UART/SPI DMA modes.
- Use DMA only for actual non-blocking transfer benefits; use `memcpy_s` for blocking memory copy.
- For Timer:
  - Timer0 is the LiteOS system clock source; do not configure it through uapi.
  - Do not call `uapi_timer_stop` or `uapi_timer_delete` inside timer callbacks.
- For PWM:
  - Do not call stop/close from interrupts.
  - Do not use duty cycle 0.
- For I2C:
  - Initialize before changing baudrate.
  - Avoid oversized transfers that can hang the bus after failures.
- For Systick:
  - It uses an internal 32 kHz clock with roughly 30 us minimum resolution; use TCXO for high precision timing.

---

## SLE / NearLink Rules

Follow the official SLE sequence unless a local sample proves a different order:

1. Register announcement/seek callbacks.
2. Register connection callbacks.
3. Register SSAP server or client callbacks.
4. Call `enable_sle`.
5. Set local address/name as needed.
6. Configure advertising or seeking parameters.
7. Start advertising for server/terminal roles, or start seeking for client/grant roles.
8. Connect, pair if required, exchange SSAP information, discover services/properties, then transfer data.

Server role:

- Use `ssaps_register_callbacks`.
- Register one SSAP server unless official docs/samples are updated.
- Define custom service/characteristic UUIDs in a header and change sample UUIDs for product work.
- Send notifications/indications only when the peer has enabled them.

Client role:

- Use `ssapc_register_callbacks`.
- Register one SSAP client unless official docs/samples are updated.
- Discover services/properties before writing or subscribing.
- Use connection IDs carefully; keep a mapping from remote address/node identity to `conn_id`.

Reliability rules:

- Restart advertising/scanning on abnormal disconnects and pairing failures.
- Do not depend on a fixed connection order unless the user-facing protocol explicitly assigns node IDs.
- Assign unique SLE MAC addresses to each server in one-to-many tests.
- Treat the documented 8 SLE connection limit as the default topology ceiling.
- Design packet formats with sequence number, timestamp, channel/device ID, payload length, and checksum/CRC for waveform data.
- For "lossless" workflows, include application-level ACK/retry or gap detection; do not assume SLE alone proves zero loss.

Recommended samples for the vital-sign sensing network:

- Start with `sle_throughput` to measure payload rate and tune MTU/connection parameters.
- Use `sle_one_to_many` to validate one gateway connecting to up to 8 patches.
- Use `sle_gate` to prototype gateway aggregation, queues, Wi-Fi, MQTT, and command downlink.

---

## Wi-Fi, MQTT, and Cloud Rules

- Use `软件开发指南` Wi-Fi flow for STA/SoftAP/coexistence and confirm asynchronous operation.
- Wi-Fi scan and connect operations are non-blocking; wait and query state rather than assuming immediate success.
- Keep network credentials and cloud secrets out of committed examples unless they are placeholders.
- Use CJSON or structured formatting for cloud payloads when data schema grows.
- Use queues between SLE callbacks and MQTT publish tasks; do not publish to cloud directly in SLE callbacks.
- For command downlink, validate command target, connection ID, payload length, and service/attribute identifier before forwarding to a SLE server.

---

## Memory, Concurrency, and Debug Rules

- FBB-RTOS uses single-process physical memory visibility without MMU isolation; any out-of-bounds write can crash the system.
- Prefer bounded APIs such as `memcpy_s`, `memset_s`, and `sprintf_s` where samples already use them.
- Check buffer lengths before copying from wireless, UART, MQTT, or sensor payloads.
- Keep task stack sizes explicit and revisit them after adding parsing, JSON, waveform buffers, or nested calls.
- For stack risk, use LiteOS stack estimation guidance and task shell output when available.
- Debug crash logs by exception type first, then task name/id, `mepc`, `ra`, `mtval`, and stack pointer.
- For watchdog timeouts, inspect high-priority loops, interrupts, CPU hogs, and feed period assumptions.
- For deadlocks, inspect lock acquisition order and avoid ABBA patterns.
- Do not enable exception interaction/debug-only options in production-style guidance unless the task is diagnostic.

---

## Medical Waveform Prototype Rules

This SDK is not a certified medical device platform by itself. For this project, implement prototype-quality sensing transport with production-minded structure:

- Separate acquisition, packetization, SLE transport, gateway aggregation, cloud uplink, and diagnostics into clear modules/tasks.
- Timestamp as close to acquisition as practical.
- Include per-sample or per-frame sequence counters.
- Preserve ordering per sensor channel.
- Detect gaps at the receiver and report them clearly.
- Avoid lossy printf-heavy telemetry in high-rate paths.
- Keep raw waveform frames binary on SLE; convert to JSON only at gateway/cloud boundaries if needed.
- Plan for backpressure: if queues fill, record the event and decide explicitly whether to drop oldest, drop newest, or block.
- Document sampling rate, bytes per sample, channels, frame interval, MTU, connection interval, and expected throughput before claiming capacity.

---

## Review Checklist

Before reporting an embedded SDK change complete:

- [ ] Official doc or local sample source was identified for the API/build pattern.
- [ ] New files are registered in the correct `CMakeLists.txt`, `Kconfig`, and/or target config.
- [ ] All wireless, driver, allocation, and copy API return values are checked.
- [ ] Hot callbacks do minimal work and hand off to a task/queue.
- [ ] Buffer lengths and task stack sizes were considered.
- [ ] SLE reconnect or rescan behavior is defined for disconnect/failure cases.
- [ ] Build command was run, or the inability to run it is reported.
- [ ] No boot/partition/NV/signing/board config was changed incidentally.

