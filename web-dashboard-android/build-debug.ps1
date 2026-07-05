param(
    [string]$GradleCommand = "gradle"
)

$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    if (Test-Path ".\gradlew.bat") {
        & .\gradlew.bat assembleDebug
    } else {
        & $GradleCommand assembleDebug
    }

    $apk = Join-Path $PSScriptRoot "app\build\outputs\apk\debug\app-debug.apk"
    if (Test-Path $apk) {
        Write-Host "APK generated: $apk"
    } else {
        Write-Host "Build finished, but APK was not found at: $apk"
    }
} finally {
    Pop-Location
}
