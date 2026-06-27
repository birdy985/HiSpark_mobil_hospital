# Merge AD8232 SLE check server into 13 sample

## Goal

Move the server-side implementation from `fbb_ws63/src/application/samples/mydemo/14_ad8232_sle_check` into `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle` while keeping the existing `13_ad8232_sle` client unchanged, then remove the `14_ad8232_sle_check` directory. Do not touch unrelated code.

## Requirements

- Treat `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle` as the final AD8232 SLE sample location.
- Keep the existing client-side behavior in `13_ad8232_sle` unchanged:
  - AD8232 sampling and `ecg_processor_process` flow stays as-is.
  - SLE packet format and client send path stay as-is.
  - CH340/local debug `display_mv` print stays as-is unless a build error forces a minimal include-only adjustment.
- Replace or extend only the server-side implementation in `13_ad8232_sle` using the server-side code from `fbb_ws63/src/application/samples/mydemo/14_ad8232_sle_check`.
- Server-side code means the `CONFIG_SAMPLE_SUPPORT_AD8232_SLE_SERVER` branch and server-only helpers needed for SLE receive, packet decode, serial-screen output, BPM update, queue handling, and logging.
- Do not migrate the 14 directory's client-side branch into 13.
- Fix migrated server dependencies so they reference the real shared AD8232 module under `../9_ad8232_adc`, not the borrowed project's stale `../0_ad8232_adc` paths.
- Preserve the current SLE packet field order and binary packet size.
- Keep the serial-screen protocol behavior from the 14 server path when moving it into 13.
- After the 13 server is self-contained and buildable, delete `fbb_ws63/src/application/samples/mydemo/14_ad8232_sle_check`.
- Do not touch unrelated samples, unrelated SDK config, or unrelated untracked project folders.

## Acceptance Criteria

- [ ] `13_ad8232_sle` client code path is unchanged except for unavoidable formatting or include consistency.
- [ ] `13_ad8232_sle` server code path receives SLE packets, decodes `display_mv`, sends samples/BPM to the serial screen, and logs the ECG CSV fields using the behavior from `14_ad8232_sle_check`.
- [ ] All migrated include paths and CMake source/header references point to existing files.
- [ ] `14_ad8232_sle_check` is removed after its server behavior has been migrated.
- [ ] `python build.py -c ws63-liteos-app -component=samples` passes from `fbb_ws63/src`.
- [ ] Final response reports how to manually verify client CH340 output versus server CSV/display output.

## Notes

- Source server path: `D:\HiSparkSDK\fbb_ws63\src\application\samples\mydemo\14_ad8232_sle_check`.
- Destination sample path: `D:\HiSparkSDK\fbb_ws63\src\application\samples\mydemo\13_ad8232_sle`.
- The 14 sample was copied from another project and currently references stale `../0_ad8232_adc` dependencies; this must not be copied blindly.
