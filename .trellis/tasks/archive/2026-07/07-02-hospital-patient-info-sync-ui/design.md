# Technical Design

## Scope

本任务横跨三个边界：

- `web-dashboard/` 静态前端：医院风格 UI、患者信息表单、患者信息接收展示。
- EMQX MQTT：复用现有云端链路，在遥测 topic 外新增患者信息 topic。
- `fbb_ws63/src/application/samples/mydemo/` 板端：服务端板子订阅患者信息 topic，解析后通过 TJC 串口屏显示。

## Current Architecture

- WS63 服务端板子发布生命体征到 `hispark/bed01/telemetry`。
- 电脑端 `index.html` 和手机端 `mobile.html` 都订阅 `hispark/bed01/telemetry`。
- 前端 MQTT 配置集中在 `web-dashboard/dashboard.js`。
- 板端 MQTT 连接、Wi-Fi、发布循环集中在 `15_emqx_telemetry/emqx_telemetry_mqtt.c`。
- 串口屏发送函数集中在 `13_ad8232_sle/tjc_display.c` / `tjc_display.h`。

## MQTT Contract

保留现有遥测 topic：

```text
hispark/bed01/telemetry
```

新增患者信息 topic：

```text
hispark/bed01/patient
```

推荐患者信息 payload：

```json
{
  "type": "patientInfo",
  "deviceId": "bed01",
  "source": "pc",
  "seq": 1,
  "ts": 1710000000000,
  "patient": {
    "name": "张三",
    "recordNo": "M20260702001",
    "gender": "男",
    "age": 56,
    "phone": "13800000000",
    "note": "术后观察"
  }
}
```

字段约定：

- `type`: 固定 `patientInfo`，便于板端和前端过滤。
- `deviceId`: 当前沿用 `bed01`。
- `source`: `pc`、`mobile` 或 `mqttx`。
- `seq`: 每个前端客户端本地递增，用于排查，不作为全局唯一顺序保证。
- `ts`: 发送端时间戳，前端用于显示更新时间；板端不依赖系统实时时钟。
- `patient`: 患者信息对象。

QoS 推荐继续使用 `0`，保持与当前遥测链路一致。前端和板端都应订阅 `hispark/bed01/patient`；前端提交后本端也通过收到 topic 消息来统一刷新显示，避免本地和远端逻辑分叉。

## Frontend Design

电脑端：

- 保留现有顶部连接状态与遥测面板。
- 调整整体配色和信息层级：更接近医院护士站/病区监测界面，使用白底、浅青/蓝绿色医疗强调色、清晰分区和紧凑信息密度。
- 增加患者信息区域：
  - 表单输入区。
  - 最新患者信息摘要区。
  - 最近同步状态/更新时间。
- 现有 ECG 波形、生命体征卡片和原始消息继续展示。

手机端：

- 保留单列 H5 结构。
- 在生命体征上方或下方增加患者信息录入卡片和最新患者信息摘要。
- 控件保持适合触控：输入框高度稳定，性别用 `select` 或分段式控件，提交按钮明确显示连接/提交状态。

`dashboard.js`：

- 将 `config.topic` 扩展为 `telemetryTopic` 和 `patientTopic`。
- `connect()` 成功后同时订阅两个 topic。
- `handleMessage(topic, message)` 根据 topic 分发：
  - `telemetryTopic` -> 现有 `updateDashboard()`。
  - `patientTopic` -> 新增 `handlePatientInfo()`。
- 新增表单绑定：
  - `bindPatientForm()`
  - `readPatientForm()`
  - `validatePatientForm()`
  - `publishPatientInfo()`
  - `updatePatientView()`
- 表单提交前确认 MQTT 已连接；失败时在页面给出明确状态，不静默丢弃。
- 备注和文本字段发布前做长度限制和去除控制字符，避免串口屏命令格式被破坏。

## Board Design

`emqx_telemetry_mqtt.c` 需要从单纯发布循环扩展为发布 + 订阅：

- 增加 `EMQX_MQTT_TELEMETRY_TOPIC` 和 `EMQX_MQTT_PATIENT_TOPIC` 常量，保留 telemetry topic 文本不变。
- MQTT 连接成功后注册消息回调并订阅 `hispark/bed01/patient`。
- 回调中只做轻量处理：
  - 过滤 topic。
  - 拷贝 payload 到固定缓冲区或消息队列。
  - 避免在回调里做长时间串口写。
- 在 MQTT 任务循环或独立处理函数中解析患者信息。

JSON 解析选择：

- 优先搜索 SDK 是否已有可用 JSON 解析库。
- 如果没有轻量 JSON 库，本任务只解析固定字段，可用受限、安全的字段提取函数，但必须限制长度、检查引号/转义、避免越界。

患者信息结构建议：

```c
typedef struct {
    char name[32];
    char record_no[32];
    char gender[8];
    char age[8];
    char phone[24];
    char note[80];
} emqx_patient_info_t;
```

年龄在前端作为数字发布，板端为了串口屏显示可转为字符串。所有字段写入结构体前都应截断并保证 `NUL` 结尾。

## TJC Serial Screen Design

在 `tjc_display.h/.c` 增加患者信息显示接口。推荐字段级接口，减少模块耦合：

```c
void tjc_display_send_patient_info(const char *name, const char *record_no,
    const char *gender, const char *age, const char *phone, const char *note);
```

TJC 命令格式沿用现有：

```text
t7.txt="张三" FF FF FF
```

用户已确认串口屏控件名：

- `t0`: 心率
- `t1`: 心电实时值
- `t2`: 血氧
- `t3`: 心率
- `t4`: 收缩压
- `t5`: 舒张压
- `t6`: 微循环
- `t7`: 姓名
- `t8`: 病历号
- `t9`: 性别
- `t10`: 年龄
- `t11`: 联系电话
- `t12`: 备注

本任务新增患者信息时只写 `t7` 至 `t12`。不得改变 `t0` 至 `t6` 的控件语义；现有心率、心电、血氧等显示链路必须保持兼容。

发送前需要转义或过滤双引号、回车、换行等会破坏 TJC 文本命令的字符。

## Compatibility

- 不改现有 `hispark/bed01/telemetry` payload 字段，避免影响已验证链路。
- 不修改其他已验证功能；前端和板端改动都应限制在医院风格、患者信息 topic、患者字段显示的必要范围内。
- 新增患者信息 topic 与遥测 topic 分离，避免板端把网页患者信息误当遥测解析。
- 前端仍可通过 `?user=账号&pass=密码` 传 MQTT 账号密码。
- 如果板端订阅失败，生命体征发布仍应继续运行，并打印错误日志。

## Risks

- 静态前端直连 MQTT 仍会暴露网页测试账号；本任务沿用现状，不解决权限问题。
- 板端 JSON 解析能力未知，执行时需要先确认 SDK 是否已有 JSON 库。
- 中文字符会增加 payload 字节数；板端缓冲区和 TJC 命令缓冲区需要按字节长度预留足够空间。


