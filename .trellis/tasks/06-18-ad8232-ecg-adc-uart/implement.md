# Implementation Plan: AD8232 ECG ADC UART Sample

Implementation will start only after user review and approval.

## Steps

1. Re-read required context before editing:
   - `fbb_ws63/src/application/samples/mydemo/8_adc/`
   - `fbb_ws63/src/application/samples/mydemo/CMakeLists.txt`
   - `fbb_ws63/src/application/samples/mydemo/Kconfig`
   - `fbb_ws63/src/application/samples/peripheral/adc/adc_demo_inc.c`
   - `fbb_ws63/src/build/config/target_config/ws63/config.py`
2. Decide whether to adapt `8_adc` directly or create a sibling sample such as `9_ad8232_adc`.
   - Recommended: create a new sibling sample to preserve the MQ2 example.
3. Implement AD8232 sample:
   - task entry via `app_run(...)`
   - ADC init with `uapi_adc_init(ADC_CLOCK_NONE)` or the clock pattern proven by local sample
   - `uapi_adc_open_channel(ADC_CHANNEL_2)`
   - loop reading `adc_port_read(ADC_CHANNEL_2, &voltage)`
   - serial/log output line: `AD8232 seq=<n> mv=<voltage>`
   - `osal_msleep(10)` or safer bring-up delay if needed
   - return-code checks
4. Register the sample using the existing `mydemo` patterns.
5. Build or report environment blocker:
   - Preferred command from SDK root: `python3 build.py ws63-liteos-app`
   - If only Windows PowerShell is available and the SDK expects WSL/Linux, document the exact blocker.
6. Provide wiring and validation instructions.
7. Include final HH-D02 wiring instructions from the schematic:
   - AD8232 `GND` -> HH-D02 `J6-16` GND
   - AD8232 `3.3` -> HH-D02 `J6-2` 3V3
   - AD8232 `OUTPUT` -> HH-D02 `J6-7` / `GPIO_09` / `ADC2`
   - AD8232 `LO+` / `LO-` left unconnected for MVP
   - AD8232 `ISDN` left unconnected first, tied to 3.3 V only if needed

## Validation Plan

Static/source validation:

- Verify the same ADC channel is opened and read.
- Verify no UART0/UART2 pinmux changes were added.
- Verify task has a bounded sleep.
- Verify API return values are checked.

Board validation:

1. Flash firmware.
2. Open the board serial log at the SDK/default baud rate.
3. Confirm lines similar to:

   ```text
   AD8232 seq=1 mv=...
   AD8232 seq=2 mv=...
   ```

4. Tie `IO_9` / ADC input to GND briefly through a safe test lead and confirm values move near 0 mV.
5. Apply a known safe voltage below 3.3 V to `IO_9` and confirm readings roughly match.
6. Connect AD8232:
   - common GND
   - AD8232 `3.3` to HH-D02 `J6-2` 3V3
   - AD8232 `OUT` to HH-D02 `J6-7` / `GPIO_09` / `ADC2`
   - leave `LO+` / `LO-` unconnected first
   - keep `ISDN` high if the module does not run with it left open
7. Confirm values vary when electrodes are attached.

## Risk Points

- AD8232 output over 3.3 V can damage the WS63 ADC input.
- Do not use Kaihong header numbering for HH-D02. HH-D02 wiring must follow `HH-D02开发板原理图.pdf`.
- Serial printing at 100 Hz may be too noisy for later waveform-quality work. It is acceptable for first bring-up.
- The uploaded PDF still needs manual review because local PDF text extraction was not available.
