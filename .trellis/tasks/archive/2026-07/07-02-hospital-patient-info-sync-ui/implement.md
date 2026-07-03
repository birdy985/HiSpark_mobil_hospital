# Implementation Plan

## Phase 0: Pre-Edit Checks

- [x] 用户已下达执行命令，任务已进入 in_progress。
- [x] 读取 `trellis-before-dev`，再读取相关前端/后端规范。
- [x] 确认当前 git 状态，未覆盖无关改动。
- [x] 使用用户已确认的串口屏控件名：`t7` 姓名、`t8` 病历号、`t9` 性别、`t10` 年龄、`t11` 联系电话、`t12` 备注；未改动 `t0` 至 `t6` 的既有/预留生命体征控件语义。

## Phase 1: Frontend UI and Patient Sync

- [x] 更新 `web-dashboard/index.html`：医院病区风格顶部与信息布局；新增患者信息表单；新增最新患者信息展示区；保留现有遥测显示节点 ID 或同步更新 JS 绑定。
- [x] 更新 `web-dashboard/mobile.html`：手机端同样包含患者信息表单和最新患者信息展示；保持窄屏下输入、提交、状态展示可用。
- [x] 更新 `web-dashboard/styles.css`：调整医院风格配色、间距、卡片、表单、移动端布局；保证按钮、输入框、状态文本在手机和电脑端不溢出。
- [x] 更新 `web-dashboard/dashboard.js`：将遥测 topic 和患者信息 topic 分开配置；连接成功后订阅两个 topic；根据 topic 分发遥测和患者信息；新增表单读取、校验、发布、状态显示；新增患者信息接收展示。

## Phase 2: Board MQTT Downlink

- [x] 搜索 SDK/工程是否已有 JSON 解析工具；确认使用已注册的 cJSON。
- [x] 更新 `15_emqx_telemetry/emqx_telemetry_mqtt.c`：重命名/新增 telemetry 和 patient topic 常量；连接后注册 MQTT message callback；订阅 `hispark/bed01/patient`；接收 payload 后解析固定患者信息字段；打印收到的患者信息摘要日志；调用串口屏患者信息显示接口。
- [x] 未更新 `15_emqx_telemetry/Kconfig`：沿用现有 EMQX publisher 开关，保持最小改动。
- [x] 未新增 C 文件，无需更新 `15_emqx_telemetry/CMakeLists.txt`。

## Phase 3: TJC Serial Screen Patient Display

- [x] 更新 `13_ad8232_sle/tjc_display.h`：声明患者信息显示接口。
- [x] 更新 `13_ad8232_sle/tjc_display.c`：增加 `t7` 至 `t12` 患者字段文本控件名常量；增加安全发送文本字段的 helper；增加患者信息批量刷新函数；保持现有波形、BPM、血氧等功能逻辑不变。
- [x] 检查并扩大 TJC 命令缓冲区，过滤会破坏命令格式的字符。

## Phase 4: Documentation

- [x] 更新 `web-dashboard/README.md`：新增患者信息 topic；新增电脑端/手机端互相同步验证步骤；新增 MQTTX 发布患者信息测试 payload；新增 WS63 订阅和串口屏显示验证日志；说明串口屏控件名映射为 `t7` 至 `t12`。

## Validation Commands

- [ ] 前端静态检查：手动打开 `web-dashboard/index.html` 和 `web-dashboard/mobile.html`，确认无控制台错误。
- [ ] 本地静态服务：

```powershell
cd web-dashboard
python -m http.server 8080
```

- [ ] 电脑端访问：

```text
http://localhost:8080/index.html?user=网页测试用户&pass=网页测试密码
```

- [ ] 手机端访问：

```text
http://电脑IP:8080/mobile.html?user=网页测试用户&pass=网页测试密码
```

- [ ] MQTTX 订阅：

```text
hispark/bed01/#
```

- [ ] MQTTX 发布患者信息：

```json
{
  "type": "patientInfo",
  "deviceId": "bed01",
  "source": "mqttx",
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

- [ ] WS63 构建命令：执行前根据当前 SDK 工具链和项目 README 确认；至少需要验证 `mydemo` 相关配置能编译通过。
- [ ] 板端运行验证：看到 `[EMQX] mqtt connected`；看到患者 topic 订阅成功日志；任一前端提交后，串口日志打印患者信息摘要；串口屏对应字段刷新；遥测发布和前端生命体征显示继续正常。

## Rollback Points

- 前端回滚：还原 `web-dashboard/index.html`、`mobile.html`、`styles.css`、`dashboard.js`、`README.md`。
- 板端回滚：还原 `15_emqx_telemetry/*` 和 `13_ad8232_sle/tjc_display.*`。
- 若板端订阅导致 MQTT 发布不稳定，应先禁用患者下行订阅，保留已验证的 telemetry 发布链路。

## Review Gate Before Starting

- [ ] 用户确认可以开始执行。
- [ ] 当前任务通过 `task.py start` 激活为 `in_progress` 后再改业务代码。


