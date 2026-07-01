# Implementation Plan

## Pre-Implementation Checks

- Read backend/frontend spec indexes and required docs before editing:
  - `.trellis/spec/backend/index.md`
  - `.trellis/spec/frontend/index.md`
  - Cross-layer guide if changing payload contracts.
- Inspect the active WS63 sample currently built by `birdy.hiproj` / CMake.
- Inspect:
  - `fbb_ws63/src/application/samples/wifi/mqtt_sample/`
  - `fbb_ws63/src/application/samples/wifi/sta_sample/`
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/`
  - relevant blood oxygen integration files once located.

## Ordered Work

1. Confirm the exact WS63 application/sample that should own the MQTT telemetry upload.
2. Define the telemetry payload struct/string builder for:
   - `dispplay_mv`
   - `heartRate`
   - `spo2`
   - timestamp or sequence.
3. Add or adapt MQTT connection code for EMQX:
   - host from `EMQX上云.txt`
   - port `8883`
   - username `ws63_server`
   - password read from the local config file during implementation, not repeated in status messages.
4. Publish sample telemetry first, then wire real current sensor variables.
5. Add desktop web page for real-time display via `wss://<host>:8084/mqtt`.
6. Add mobile H5 page or responsive entry based on final user approval.
7. Validate with MQTTX:
   - WS63 publishes to `hispark/bed01/telemetry`.
   - MQTTX receives under `hispark/bed01/#`.
8. Validate browser/H5:
   - MQTTX sample payload updates the UI.
   - WS63 real payload updates the UI.

## Validation Commands / Manual Checks

- Build command to be confirmed from existing project workflow before execution.
- MQTTX manual check:
  - subscribe `hispark/bed01/#`
  - verify telemetry payloads appear.
- Browser manual check:
  - open desktop page
  - open mobile page or mobile viewport
  - confirm connected state and live values.

## Risks

- EMQX Serverless requires TLS on port `8883`; WS63 MQTT TLS configuration may need CA/certificate handling.
- Sensor values may live in separate samples; merging ECG and SpO2 into one WS63 app may require identifying current data ownership.
- The requested field name `dispplay_mv` may differ from code spelling. Frontend should tolerate both.
- Static frontend credentials are visible in browser source; acceptable for demo only.

## Start Gate

Do not run `task.py start` and do not edit implementation files until the user approves this task说明.
