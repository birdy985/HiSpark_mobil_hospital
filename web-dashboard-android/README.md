# HiSpark Health H5 Android Demo

This is a minimal Android WebView wrapper for `web-dashboard/mobile.html`.

## Build

Install Android Studio or Android SDK command-line tools, then run from this directory:

```powershell
.\build-debug.ps1
```

The debug APK will be generated at:

```text
app/build/outputs/apk/debug/app-debug.apk
```

## Notes

- The H5 files are bundled under `app/src/main/assets/`.
- The app needs network access because `mobile.html` loads MQTT.js from `https://unpkg.com/` and connects to MQTT over WebSocket.
- MQTT credentials are still controlled by `dashboard.js` or URL parameters in the H5 page.
- This machine did not have Gradle or Android SDK installed when the wrapper project was created, so APK compilation must be run after those tools are installed.
