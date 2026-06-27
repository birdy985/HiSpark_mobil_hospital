# Design: Direct display_mv Serial Screen Adaptation

## 总体设计

现有屏幕已经读取 `display_mv` 字段绘图，因此本任务不新增 `screen_y` 字段，而是在客户端发送前把 `display_mv` 转换为串口屏纵轴坐标。

```text
AD8232 ADC mV every 10 ms
  -> ecg_processor_process()
  -> output.display_mv is still ECG mV offset internally
  -> tx_sample = output
  -> tx_sample.display_mv = ecg_screen_map_y(output.display_mv)
  -> ecg_sle_encode_sample(&tx_sample)
  -> SLE SSAP write at screen send interval
  -> server decode
  -> CSV disp_mv is 0..255 screen Y
```

关键边界：不修改 `ecg_processor_process()` 的语义，只修改发送副本 `tx_sample.display_mv`。

## 纵轴映射

串口屏纵轴范围为 `0..255`，推荐基线中心为 `128`。

推荐常量：

```c
#define ECG_SCREEN_Y_MIN          0
#define ECG_SCREEN_Y_MAX          255
#define ECG_SCREEN_Y_CENTER       128
#define ECG_SCREEN_MV_FULL_SCALE  640
#define ECG_SCREEN_Y_INVERT       1
```

推荐公式：

```text
scaled = display_mv * 127 / ECG_SCREEN_MV_FULL_SCALE
if ECG_SCREEN_Y_INVERT:
    y = 128 - scaled
else:
    y = 128 + scaled
y = clamp(y, 0, 255)
```

说明：

- `display_mv` 输入仍是 ECG 处理后的 mV 偏移值。
- `display_mv` 发送值变为屏幕 Y 坐标，类型仍可沿用 `int16_t`，但有效范围为 `0..255`。
- `ECG_SCREEN_MV_FULL_SCALE` 是幅度调节旋钮。波形太小就调小，波形贴边就调大。
- 如果屏幕坐标 0 在上方，常用 ECG 正向 R 波应使用 `128 - scaled`。

## 横轴发送频率

现有 `ECG_SLE_SAMPLE_MS = 10` 同时承担采样和发送。为让 X 轴符合 ECG 显示，设计上拆成两个概念：

- `ECG_SLE_SAMPLE_MS = 10`：ADC 采样和 ECG 处理周期，保留 100 Hz。
- `ECG_SCREEN_SEND_MS`：实际发给屏幕绘图的周期，控制横轴速度。

推荐初始值：

```c
#define ECG_SCREEN_SEND_MS 40
```

含义：每秒发送 25 个绘图点。若串口屏每收到 1 点横向推进 1 mm，则对应 `25 mm/s`。如果屏幕每点是 1 像素，需要结合屏幕像素密度调整该值。

实现策略：

- 客户端仍每 10 ms 读取和处理 ECG，用于滤波、R 峰和 BPM。
- 只有当 `now_ms - last_screen_send_ms >= ECG_SCREEN_SEND_MS` 时，才发送一帧给 SLE/串口屏。
- `seq` 建议仍跟随采样递增，服务端 gap 检测需要理解为“发送帧序号”或改用发送序号。为避免 10 ms 采样但 40 ms 发送导致服务端误报 gap，推荐发送包使用独立 `tx_seq`。

## 包格式

不新增字段，不升级包长度。继续使用当前固定 32 字节包：

```text
seq, timestamp_ms, voltage_mv, baseline_mv, ecg_mv, filtered_mv, display_mv, r_peak, quality, rr_interval_ms, bpm
```

字段语义调整：

- `display_mv` 在线路包和服务端 CSV 中表示串口屏 Y 坐标，范围 `0..255`。
- `ecg_mv` 和 `filtered_mv` 仍保留 mV 调试值。
- 如需看原始平滑显示 mV，可临时从客户端日志或后续新增调试字段解决；本任务优先满足串口屏兼容。

## 代码边界

建议新增：

- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/ecg_screen_map.h`
- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/ecg_screen_map.c`

建议修改：

- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/main.c`
  - 拆分采样周期和发送周期。
  - 发送副本中改写 `display_mv`。
  - 使用独立发送序号避免服务端 gap 误报。
- `fbb_ws63/src/application/samples/mydemo/13_ad8232_sle/CMakeLists.txt`
  - 加入 `ecg_screen_map.c`。

不建议修改：

- `9_ad8232_adc/ecg_processor.*`
- `13_ad8232_sle/ecg_sle_packet.*` 的包长度和字段顺序。
- `sle_uart_client/`、`sle_uart_server/` 连接/传输代码。

## 服务端输出

CSV 表头可以保持现有名字 `disp_mv`，但实现注释和规划明确：此列现在是串口屏 Y 坐标。

可选更清晰的表头：

```text
ECG_SLE,seq,t_ms,raw_mv,base_mv,ecg_mv,filt_mv,disp_y,r,rr_ms,bpm,q
```

为减少串口屏侧字段绑定变化，默认不改列位置，只保证原 `disp_mv` 位置输出 `0..255`。

## 风险与取舍

- 直接改 `display_mv` 会牺牲该字段在线路上的 mV 语义，但符合当前串口屏集成方式。
- 降低发送频率会减少绘图点密度；但 ECG 处理仍保持 100 Hz，心率和滤波不跟随降低。
- 如果屏幕自身已经按 25 mm/s 定时推进，额外降到 25 Hz 可能让波形变稀疏；因此发送周期必须集中可调。

