# Design: AD8232 ECG waveform and heart-rate processing

## Architecture

Enhance the existing `fbb_ws63/src/application/samples/mydemo/9_ad8232_adc` sample in place after user approval.

Proposed module boundaries:

- `ad8232_bsp.c/.h`: unchanged hardware boundary for ADC init and millivolt reads.
- `ecg_processor.c/.h`: new signal-processing module with no direct ADC or serial dependencies.
- `main.c`: acquisition loop, timestamping, calls processor, and emits parseable serial output.

This keeps hardware access, signal processing, and transport formatting separate while staying scoped to the sample.

## Datasheet / Manual Gate

Before code implementation starts, confirm the AD8232 manuals located under `AD8232心电传感器/`:

- `AD8232.pdf`
- `AD8232_STM32套件说明书.pdf`

Current environment cannot extract reliable PDF text. Execution should not claim datasheet-backed details until the key points are checked by a working PDF reader/tool or manual inspection:

- AD8232 output bias and expected mid-supply baseline
- output range when powered at 3.3 V
- `LO+` / `LO-` lead-off signal behavior
- `SDN` / `ISDN` shutdown behavior
- any recommended ECG electrode and filtering notes relevant to single-lead monitoring

## Data Contract

Input sample:

```c
typedef struct {
    uint32_t seq;
    uint32_t timestamp_ms;
    uint16_t voltage_mv;
} ecg_input_sample_t;
```

Output sample:

```c
typedef enum {
    ECG_QUALITY_GOOD = 0,
    ECG_QUALITY_NOISY,
    ECG_QUALITY_SATURATED,
    ECG_QUALITY_FLATLINE,
    ECG_QUALITY_NO_R_PEAK,
} ecg_signal_quality_t;

typedef struct {
    uint32_t seq;
    uint32_t timestamp_ms;
    uint16_t voltage_mv;
    int16_t baseline_mv;
    int16_t ecg_mv;
    int16_t filtered_mv;
    int16_t display_mv;
    uint8_t r_peak;
    uint16_t rr_interval_ms;
    uint16_t bpm;
    ecg_signal_quality_t quality;
} ecg_monitor_sample_t;
```

## Processing Flow

1. Acquire ADC millivolts from `ADC_CHANNEL_2`.
2. Timestamp close to acquisition.
3. Initialize baseline from early samples.
4. Track baseline with a slow IIR so QRS peaks do not drag it quickly.
5. Compute `ecg_mv = voltage_mv - baseline_mv`.
6. Apply lightweight filters:
   - slow baseline removal for drift/high-pass behavior
   - low-pass or display smoothing for visible waveform
   - optional slope/step limiting for isolated ADC spikes, only on display output
7. Detect R peaks:
   - use filtered ECG absolute amplitude so either positive or negative electrode orientation can be detected
   - confirm a local extremum by waiting for the filtered signal to turn back
   - adaptive threshold based on recent peak/noise levels
   - refractory period to avoid double-counting one QRS
   - reject implausible RR intervals
8. Compute BPM:
   - `bpm = 60000 / rr_ms`
   - keep `bpm = 0` until a credible interval exists
9. Assess signal quality:
   - rails / near-rails -> `SATURATED`
   - too-small dynamic range -> `FLATLINE`
   - long interval without R peak -> `NO_R_PEAK`
   - excessive sample jumps -> `NOISY`
   - otherwise `GOOD`
   - when a window reports `NO_R_PEAK`, reset the peak/noise threshold estimator so detection can recover instead of staying stuck

## Output Strategy

For plotting, avoid sending only raw 0-3300 mV values. Output should identify a display waveform channel centered around 0 mV.

Recommended serial format:

```text
ECG,<seq>,<t_ms>,<raw_mv>,<base_mv>,<ecg_mv>,<filt_mv>,<disp_mv>,<r>,<rr_ms>,<bpm>,<q>
```

If serial throughput makes sampling jitter too high, split output into:

- high-rate waveform line: `W,<seq>,<t_ms>,<disp_mv>,<q>`
- lower-rate diagnostic line: `D,<seq>,<t_ms>,<raw_mv>,<base_mv>,<ecg_mv>,<filt_mv>,<disp_mv>,<r>,<rr_ms>,<bpm>,<q>`

Plotting guidance:

- x-axis: use `t_ms`, not assumed fixed spacing
- y-axis: use `disp_mv` or `ecg_mv`, not raw ADC mV
- ECG convention target: 25 mm/s horizontal, 10 mm/mV vertical on the PC-side renderer

## Sampling Rate

Current sample uses 10 ms delay, nominal 100 Hz. Plan:

- Start with 100 Hz for stability if serial output is full diagnostic CSV.
- Prefer 250 Hz if output is reduced to waveform-only lines or buffered transport.
- Always include timestamps because RTOS sleep and serial `printf` can stretch intervals.

## Medical Meaning Boundary

This design supports:

- baseline-centered ECG-like waveform
- R-peak timing
- RR interval
- approximate BPM
- signal quality flags

This design explicitly does not support:

- clinical diagnosis
- arrhythmia classification
- ST/QT analysis
- multi-lead ECG interpretation
- certified alarm behavior

## Rollback

Rollback should be sample-local:

- restore `main.c`
- remove `ecg_processor.c/.h`
- remove `ecg_processor.c` from sample `CMakeLists.txt`
