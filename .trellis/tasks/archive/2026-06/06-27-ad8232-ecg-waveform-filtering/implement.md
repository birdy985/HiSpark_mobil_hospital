# Implementation Plan: AD8232 ECG waveform filtering

## Checklist

1. Load project coding guidance before editing:
   - `.trellis/spec/backend/index.md`
   - `.trellis/spec/backend/ws63-sdk-coding-standard.md`
   - `.trellis/spec/backend/quality-guidelines.md`
2. Inspect current files:
   - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/ecg_processor.c`
   - `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc/ecg_processor.h`
   - `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/main.c`
   - `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/ecg_sle_packet.*`
3. Update `ecg_processor.c` only if possible:
   - add input spike guard state
   - tune baseline and filter constants
   - feed `display_mv` from `filtered_mv`
   - clamp display amplitude and step changes
   - keep debug fields populated
4. Update the SLE client path in `13_ad8232_sle/main.c`:
   - keep the existing SLE send path intact
   - after computing the final display value, print the same value locally through CH340/debug serial with `printf("%d", disp_mv)` semantics
   - do not add CSV prefixes or new packet fields for this local validation output
5. Avoid changing packet structs or SLE encode/decode unless required.
6. Build or run the closest available compile/check command.
7. Check `git diff` for unintended protocol or config changes.
8. Report hardware test steps:
   - same electrode placement before/after
   - watch `quality`, `bpm`, and screen waveform
   - confirm CH340/debug serial prints the final `disp_mv` values
   - retry with battery/clean power if the trace remains noisy

## Risky Points

- Over-smoothing can hide R peaks and destabilize BPM.
- Aggressive spike rejection can treat real QRS slopes as noise.
- Changing `display_mv` scale can break the screen's visual range.
- Packet format changes would break the already-bound serial screen.
- Per-sample `printf` can slow the 10 ms client loop if the debug serial path blocks.

## Rollback

- Revert changes in `9_ad8232_adc/ecg_processor.c`.
- Revert CH340/debug serial print changes in `13_ad8232_sle/main.c`.
- If packet files were touched, revert them first and verify `ECG_SLE_PACKET_LEN` matches the previous value.

## Planning Status

Ready for user review. Do not run implementation until the user approves.
