# Design: AD8232 ECG waveform filtering

## Approach

Keep the existing acquisition and SLE transport shape. Optimize the reusable ECG processor so both `9_ad8232_adc` and `13_ad8232_sle` benefit from the same cleaned signal path.

The main signal-cleanup change is inside `9_ad8232_adc/ecg_processor.c`. The SLE packet format remains unchanged; `display_mv` continues to be the screen-bound field. `13_ad8232_sle/main.c` also needs a minimal client-side CH340/debug serial print of the final display value for direct validation.

## Data Flow

```
AD8232 ADC millivolts at 100 Hz
  -> ecg_processor_process()
     -> raw clamp and spike guard
     -> baseline tracker / high-pass extraction
     -> low-pass smoothing for ECG display
     -> R-peak detection from filtered signal
     -> display_mv from cleaned filtered signal with step/amplitude limits
  -> ecg_sle_encode_sample()
  -> SLE write to server
  -> client local CH340/debug serial printf of final disp_mv
  -> server CSV / serial screen
```

## Processor Changes

- Add a tiny input de-spike stage before baseline tracking:
  - compare current `voltage_mv` with previous accepted voltage
  - if one sample jumps far beyond plausible ECG movement, limit or replace it with the previous accepted value
  - keep this conservative so real QRS slopes are not erased
- Tune baseline handling:
  - keep a slow baseline tracker for DC and respiration/contact drift
  - prevent a single spike from pulling the baseline
- Make `filtered_mv` the primary cleaned ECG value:
  - apply low-pass smoothing to the baseline-subtracted ECG signal
  - use the filtered signal for R-peak detection and display target
- Generate `display_mv` from `filtered_mv`, not from raw `ecg_mv`:
  - retain step limiting
  - add bounded amplitude clamp for screen stability
  - avoid auto-scaling that changes the meaning of the existing field

## Client CH340 Validation Output

- In the `CONFIG_SAMPLE_SUPPORT_AD8232_SLE_CLIENT` path, after the sample has its final display value, print that value locally over the board debug serial/CH340 path.
- Keep this output minimal for validation: `printf("%d", disp_mv)` semantics, with no CSV prefix and no packet format change.
- `disp_mv` should mean the same value that is sent in the packet as `display_mv`, after software filtering and any screen/display mapping.
- The print must not replace SLE sending; it is a duplicate local tap of the client-side data path.
- Risk: printing every 10 ms can add serial overhead. For the requested minimum validation, keep it simple first and only throttle or guard it if it breaks the sample loop.

## Compatibility

- `ecg_monitor_sample_t` stays unchanged.
- `ecg_sle_packet.*` stays unchanged unless validation reveals an unrelated bug.
- `13_ad8232_sle/main.c` needs only local client-side debug serial output; no SLE protocol changes.
- Server CSV header remains the same.

## Trade-Offs

- More smoothing improves visual stability but can round off sharp QRS peaks.
- Less smoothing preserves morphology but leaves more noise on the serial screen.
- Recommended default: moderate smoothing and conservative spike suppression, because the immediate user pain is unreadable display noise.

## Validation

- Build or at least run the relevant SDK compile command if available locally.
- Inspect code for integer overflow and signed/unsigned conversions.
- Confirm packet length and CSV field order are unchanged.
- On hardware, compare the serial screen before/after using the same electrode placement.
- On client CH340/debug serial, confirm the output is the final `disp_mv` stream in minimal numeric format.
