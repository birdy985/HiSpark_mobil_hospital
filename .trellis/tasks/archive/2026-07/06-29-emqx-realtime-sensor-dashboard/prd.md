# EMQX realtime sensor dashboard

## Goal

Build the first cloud display slice for the competition demo: the WS63 server board publishes patient sensor telemetry to EMQX, and both desktop web and mobile H5 pages display the data in real time.

This task intentionally excludes patient case/info downlink and serial-screen case display. Those will be planned after telemetry upload and dashboards are stable.

## Confirmed Facts

- EMQX Cloud has already been configured and verified with MQTTX.
- EMQX connection details are stored locally in `D:\HiSparkSDK\EMQX上云.txt`.
- EMQX host is `q67a1139.ala.cn-shenzhen.emqxsl.cn`.
- MQTT over TLS/SSL port is `8883` for WS63 device access.
- WebSocket over TLS/SSL port is `8084` for browser/H5 access.
- Existing MQTT topics include:
  - `hispark/bed01/telemetry` for sensor telemetry upload.
  - `hispark/bed01/status`, `hispark/bed01/alert` are available for later state/alarm messages.
  - Patient downlink topics exist but are out of scope for this task.
- WS63 Wi-Fi connection has already been tested successfully by the user.
- The repository contains WS63 Wi-Fi/MQTT samples:
  - `fbb_ws63/src/application/samples/wifi/mqtt_sample/`
  - `fbb_ws63/src/application/samples/wifi/sta_sample/`
- The repository contains AD8232 ECG sample code:
  - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/`
- Relevant telemetry fields requested by the user:
  - ECG real-time millivolt/display value: `dispplay_mv` (preserve the user's field spelling unless code evidence proves the existing symbol is different).
  - Heart rate.
  - Blood oxygen SpO2.

## Requirements

- WS63 publishes telemetry JSON to `hispark/bed01/telemetry` through EMQX.
- Telemetry JSON contains at minimum:
  - `deviceId`
  - `dispplay_mv`
  - `heartRate`
  - `spo2`
  - `ts` or another timestamp/sequence field if available.
- Desktop page connects to EMQX through `wss://<host>:8084/mqtt`.
- Mobile H5 page connects to EMQX through `wss://<host>:8084/mqtt`.
- Desktop and mobile pages subscribe to `hispark/bed01/telemetry`.
- Both pages update displayed values immediately when new telemetry arrives.
- UI clearly shows connection state, last update time, latest ECG value, heart rate, and SpO2.
- Pages should be usable as static files unless repository evidence shows an existing frontend build system should be used.
- EMQX credentials must not be printed in final answers. If credentials must be used in frontend demo code, keep them isolated in a config section/file with clear placeholder/edit points.

## Out Of Scope

- Sending virtual patient case information from web/H5 to WS63.
- WS63 receiving `hispark/bed01/patient/set`.
- Serial screen case display.
- User login, role permissions, database storage, historical chart backend, or real medical privacy workflows.
- Multi-bed management beyond `bed01`.

## Acceptance Criteria

- [ ] With MQTTX publishing a sample telemetry message to `hispark/bed01/telemetry`, the desktop page displays updated `dispplay_mv`, heart rate, and SpO2.
- [ ] With MQTTX publishing the same sample message, the mobile H5 page displays updated `dispplay_mv`, heart rate, and SpO2.
- [ ] When WS63 publishes telemetry to EMQX, MQTTX subscribed to `hispark/bed01/#` receives it.
- [ ] When WS63 publishes telemetry to EMQX, desktop and mobile pages update without manual refresh.
- [ ] Page connection status changes visibly for disconnected, connecting, connected, and error states.
- [ ] The implementation does not add patient-info downlink behavior in this task.
- [ ] The implementation leaves existing unrelated user changes untouched.

## Open Question Before Implementation

- Should the frontend be a single responsive HTML file, or two separate files for desktop and mobile H5?

Recommended answer: create one responsive `index.html` for fast testing plus a phone-focused `mobile.html` if packaging needs a dedicated entry. This is slightly more work than one file, but makes both demo scenarios cleaner.
