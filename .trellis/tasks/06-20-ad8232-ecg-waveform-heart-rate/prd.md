# AD8232 ECG waveform and heart-rate processing

## Goal

Use the existing AD8232 ADC millivolt stream on HH-D02 / WS63 to produce ECG-oriented data that can be drawn as a waveform and used to estimate heart rate.

The implementation should remain a prototype monitoring aid. It must make the waveform and heart-rate calculation more medically meaningful than raw 0-3300 mV values, but it must not claim diagnostic-grade ECG interpretation.

Do not implement yet. This task is in planning until the user reviews and approves the plan.

## Confirmed Evidence

- Current WS63 AD8232 sample exists at `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc`.
- Current sample reads `ADC_CHANNEL_2` through `ad8232_read_mv()` and prints `seq,voltage_mv` every 10 ms.
- Current BSP uses HH-D02 `J6-7` / `GPIO_09` / `ADC2` and logs this wiring.
- Historical AD8232 task `06-18-ad8232-ecg-adc-uart` documented the hardware bring-up:
  - AD8232 `OUT` -> HH-D02 `J6-7` / `GPIO_09` / `ADC2`
  - AD8232 `3.3` -> HH-D02 `J6-2` / `3V3`
  - AD8232 `GND` -> HH-D02 `J6-16` / `GND`
- Local AD8232 material is present:
  - `AD8232心电传感器/AD8232.pdf`
  - `AD8232心电传感器/AD8232_STM32套件说明书.pdf`
  - `AD8232心电传感器/采集上传V2/采集上传V2/`
- Local PDF text extraction is currently blocked: no `pdftotext`, PyPDF2, pypdf, pdfplumber, PyMuPDF, pdfminer, mutool, pdfinfo, or pdftohtml is available in this environment. The files were located, but key datasheet parameters still need manual/tool-assisted confirmation before implementation starts.
- The STM32 reference project reads ADC channel 0 and prints one raw ADC value every 10 ms; it does not include medical ECG processing.
- WS63 project guideline for medical waveform prototypes requires timestamping close to acquisition, sequence counters, gap/quality detection, and avoiding printf-heavy high-rate telemetry.

## Requirements

- Preserve the existing HH-D02 wiring and `ADC_CHANNEL_2` default.
- Convert each 0-3300 mV ADC reading into ECG-oriented relative samples:
  - estimate the AD8232 DC bias/baseline
  - subtract baseline so the ECG waveform is centered near 0 mV
  - filter enough to reduce drift and high-frequency noise without flattening QRS complexes
- Provide output that a PC plotting tool can draw as an ECG waveform:
  - sequence number
  - timestamp in milliseconds
  - raw ADC millivolts
  - baseline millivolts
  - ECG relative millivolts
  - display waveform millivolts
  - R-peak flag
  - RR interval in milliseconds
  - BPM
  - signal quality
- Calculate heart rate from detected R peaks:
  - use adaptive thresholding
  - use a refractory period to avoid double-counting the same QRS complex
  - keep BPM invalid/zero until at least one credible RR interval is available
  - reject implausible RR intervals
- Include signal quality states at minimum:
  - `GOOD`
  - `NOISY`
  - `SATURATED`
  - `FLATLINE`
  - `NO_R_PEAK`
- Keep the output machine-parseable and suitable for plotting.
- Keep implementation scoped to the existing `9_ad8232_adc` sample unless planning review changes scope.
- Use integer or fixed-point processing suitable for embedded execution.
- Avoid disease diagnosis, arrhythmia classification, ST segment analysis, QT analysis, cloud upload, or multi-lead ECG claims.
- Before implementation, complete AD8232 datasheet/manual confirmation for:
  - output bias behavior
  - output swing / ADC safety range when powered from 3.3 V
  - lead-off pins `LO+` / `LO-`
  - shutdown pin `SDN` / `ISDN`
  - expected electrode/contact caveats

## Acceptance Criteria

- [ ] User reviews and approves `prd.md`, `design.md`, and `implement.md` before implementation starts.
- [ ] AD8232 datasheet/manual key points are confirmed before code changes begin.
- [ ] Firmware still initializes HH-D02 `J6-7` / `GPIO_09` / `ADC2` successfully.
- [ ] Serial output remains machine-parseable and includes raw and processed ECG fields.
- [ ] Baseline converges near the AD8232 DC bias while ECG-relative output oscillates around 0 mV.
- [ ] Display waveform can be plotted without treating raw 0-3300 mV values as the ECG y-axis.
- [ ] R peaks are marked intermittently when electrode signal contains clear heartbeats.
- [ ] BPM becomes non-zero only after credible RR intervals are detected.
- [ ] Saturated input is reported as `SATURATED`.
- [ ] Nearly constant signal is reported as `FLATLINE` or `NO_R_PEAK`.
- [ ] No diagnostic-grade medical claims are added.
- [ ] Build is attempted with the WS63 SDK command, or the exact build-environment blocker is reported.

## Notes

- This task should produce medically meaningful monitoring signals, not a certified medical device.
- A 10 ms loop is nominally 100 Hz and can demonstrate heart-rate estimation, but 250 Hz is a better ECG monitoring target if serial/log throughput can support it.
- If serial output limits actual sampling rate, the implementation should prefer timestamped samples and/or decimated diagnostics over assuming a fixed interval.

## Open Questions

- None that blocks planning. The next blocking item is evidence-based: confirm the AD8232 PDF/manual key points before implementation.
