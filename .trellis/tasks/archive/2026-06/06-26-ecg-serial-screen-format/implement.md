# Implementation Plan: Direct display_mv Serial Screen Adaptation

## 前置条件

- 用户确认本次更新后的 `prd.md`、`design.md` 和 `implement.md`。
- 横轴推进模型已确认：串口屏每收到 1 个数据就绘制 1 个点。
- 进入实现前执行 `task.py start`。
- 写代码前加载 `trellis-before-dev` 并读取 backend 规范。

## 实施步骤

1. 读取实现相关规范和代码：backend 规范、`13_ad8232_sle/main.c`、`ecg_sle_packet.*`、`CMakeLists.txt`。
2. 新增 `ecg_screen_map.h/.c`：
   - 提供 `int16_t ecg_screen_map_display_mv(int16_t display_mv)`。
   - 返回值范围固定 `0..255`，类型用 `int16_t` 以兼容现有包字段。
   - 集中定义中心、上下限、满幅 mV、方向常量。
3. 修改客户端发送循环：
   - 保持 ADC/ECG 处理周期 `10 ms`。
   - 新增 `ECG_SCREEN_SEND_MS` 控制发送周期，默认建议 `40 ms`。
   - 每次采样都执行 `ecg_processor_process()`。
   - 到达发送周期时复制 `output` 到 `tx_sample`。
   - 将 `tx_sample.display_mv` 改写为 `ecg_screen_map_display_mv(output.display_mv)`。
   - 使用独立 `tx_seq` 写入 `tx_sample.seq`，避免服务端因抽帧发送误报 gap。
4. 保持包格式不变：
   - 不新增 `screen_y`。
   - 不修改 `ECG_SLE_PACKET_LEN`。
   - 不修改 encode/decode 字段顺序。
5. 服务端输出：
   - 默认保留原 CSV 字段位置，`disp_mv` 输出 0..255。
   - 可将表头改为 `disp_y`，但若担心串口屏按列名绑定，则保持 `disp_mv`。
6. 更新 `CMakeLists.txt`，加入 `ecg_screen_map.c`。
7. 静态检查：
   - 确认 `display_mv` 发送值不会越界。
   - 确认发送节奏不会阻塞 ADC 采样处理。
   - 确认服务端 gap 检测不因降频发送误报。
8. 构建验证。

## 验证命令

```bash
python3 build.py ws63-liteos-app
```

必要时：

```bash
python3 build.py -c ws63-liteos-app
```

切换角色时：

```bash
python3 build.py -c ws63-liteos-app menuconfig
```

## 硬件验证

1. 服务端启动后确认 CSV 正常输出。
2. 客户端启动后确认 `disp_mv` 始终在 `0..255`。
3. 串口屏继续读取原 `display_mv/disp_mv` 字段绘图。
4. 观察基线是否接近 128。
5. 观察 R 波方向是否正确；若上下反了，调整方向常量。
6. 观察横向速度是否接近屏幕 `25 mm/s` 标识；若太快/太慢，调整 `ECG_SCREEN_SEND_MS`。
7. 确认心率 `bpm` 仍稳定输出。

## 回退点

- 删除 `ecg_screen_map.*`。
- 恢复客户端每次采样即发送。
- 恢复发送包中 `display_mv` 使用原始 mV 值。
- 保持星闪连接和服务端接收代码不变。

## 审查门槛

- 没有新增 `screen_y` 字段。
- 没有改变 SLE 包长度和字段顺序。
- `display_mv` 在线路上的新语义有明确注释。
- ADC 处理频率和屏幕发送频率分离。
- 发送序号与服务端 gap 检测语义一致。
- 映射常量集中，便于现场调试。


## 实施记录

- 已新增 `ecg_screen_map.h/.c`，集中实现 `display_mv` 到 `0..255` 串口屏 Y 坐标的映射。
- 已保持 ADC/ECG 处理周期 `ECG_SLE_SAMPLE_MS = 10`。
- 已新增屏幕绘图发送周期 `ECG_SCREEN_SEND_MS = 40`，串口屏每收到一个数据绘制一个点，因此默认约 25 点/秒。
- 已在客户端发送副本中将 `display_mv` 改写为屏幕 Y 坐标，处理器内部 `output.display_mv` 仍保留 mV 语义。
- 已使用独立 `tx_seq` 作为发送帧序号，避免降频发送导致服务端 gap 检测误报。
- 未修改 SLE 包长度、字段顺序、客户端 SSAP 写入函数、服务端回调/队列/解包主流程。

## 验证记录

- `python build.py ws63-liteos-app`：失败，原因是既有 CMake 缓存生成器冲突，提示之前使用 Ninja、本次使用 Unix Makefiles。
- `python build.py -c ws63-liteos-app`：清理后进入全量构建，但失败在无关 Wi-Fi 组件 `protocol/wifi/.../hmac_auto_adjust_freq.c.obj`，报错 `The system cannot execute the specified program`。
- `python build.py ws63-liteos-app -component=samples`：通过，日志显示 `Build target:ws63_liteos_app, component:[samples] success`。
