# Design

## Architecture

The task has two delivery surfaces:

1. WS63 telemetry publisher
   - Runs on the WS63 server board after Wi-Fi connects.
   - Uses the existing MQTT client sample/library in `fbb_ws63`.
   - Publishes sensor telemetry to EMQX on `hispark/bed01/telemetry`.

2. Browser dashboard
   - Static desktop web page and mobile H5 page.
   - Uses MQTT over WebSocket TLS to connect to EMQX.
   - Subscribes to `hispark/bed01/telemetry`.
   - Parses JSON payloads and updates the UI in real time.

## Data Flow

```text
AD8232 / heart rate / SpO2 sources
  -> WS63 service-side application
  -> MQTT over TLS, port 8883
  -> EMQX Cloud
  -> MQTT over WebSocket TLS, port 8084, path /mqtt
  -> desktop page and mobile H5 page
```

## MQTT Contract

Topic:

```text
hispark/bed01/telemetry
```

Initial payload shape:

```json
{
  "deviceId": "bed01",
  "dispplay_mv": 1234,
  "heartRate": 78,
  "spo2": 98,
  "ts": 1710000000000
}
```

Compatibility rules:

- Frontend should tolerate missing optional fields and show `--` instead of crashing.
- Frontend should accept numeric strings from embedded code and coerce them for display.
- If existing WS63/ECG code uses `display_mv` instead of `dispplay_mv`, either publish both fields or normalize in the frontend. The user-requested spelling remains supported.

## Frontend Design

- Use a monitoring dashboard layout, not a landing page.
- Desktop view emphasizes scan-friendly cards/panels for ECG, heart rate, SpO2, connection state, and raw latest message.
- Mobile H5 uses a compact single-column layout suited for packaging into an app wrapper.
- Include connection controls only if needed for debugging; default should auto-connect using configured EMQX values.
- Do not implement patient case forms in this task.

## WS63 Design

- Reuse the existing Wi-Fi/MQTT sample structure where possible.
- Keep EMQX constants grouped in one local config block.
- Publish at a controlled rate so the dashboard updates smoothly without flooding EMQX.
- Start with publishing current available sensor values. If real SpO2 integration is not yet present in the same WS63 application, support a temporary stub only when clearly marked and easy to replace.

## Security And Demo Constraints

- This is a competition demo with virtual data, not real patient data.
- Browser MQTT credentials are inherently visible to users in static frontend code. For this demo that can be acceptable, but credentials should be isolated in one config location and not repeated throughout the code.
- Final response should not print passwords.

## Rollback

- WS63 changes should be isolated to the selected sample/application files and their build registration.
- Frontend changes should be additive, preferably in a new web/H5 directory, so they can be removed without touching embedded code.
