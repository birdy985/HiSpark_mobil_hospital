# HiSpark EMQX Dashboard

静态电脑端和手机 H5 页面，用于订阅 EMQX `hispark/bed01/telemetry` 并实时显示：

- `dispplay_mv` / `display_mv`
- `heartRate` / `bpm` / `hr_bpm`
- `spo2` / `spo2_x10`

同时支持电脑端和手机端录入患者信息，发布到 `hispark/bed01/patient`。任一端提交后，另一端会收到同一份患者信息，WS63 服务端板子订阅后可转发给串口屏 `t7` 至 `t12` 控件显示。

## 页面

- 电脑端：`index.html`
- 手机端 H5：`mobile.html`

## MQTT 连接

配置集中在 `dashboard.js` 顶部：

```js
host: "q67a1139.ala.cn-shenzhen.emqxsl.cn"
websocketPort: 8084
path: "/mqtt"
telemetryTopic: "hispark/bed01/telemetry"
patientTopic: "hispark/bed01/patient"
```

MQTT 账号默认使用占位值，不提交真实密码。演示时可以临时编辑 `dashboard.js` 顶部配置，或在访问 URL 后追加 `?user=账号&pass=密码`。真实项目不要把 MQTT 密码放在静态前端里。

## 电脑和手机页面验证

1. 在 `web-dashboard` 目录启动静态服务：

   ```powershell
   python -m http.server 8080
   ```

2. 电脑浏览器打开：

   ```text
   http://localhost:8080/index.html?user=网页测试用户&pass=网页测试密码
   ```

3. 手机和电脑连同一个 Wi-Fi，手机浏览器打开：

   ```text
   http://电脑IP:8080/mobile.html?user=网页测试用户&pass=网页测试密码
   ```

4. 打开 MQTTX，连接你的 EMQX：

   ```text
   协议：wss
   地址：q67a1139.ala.cn-shenzhen.emqxsl.cn
   端口：8084
   路径：/mqtt
   用户名：使用 EMQX 里配置的网页测试用户
   ```

5. MQTTX 订阅：

   ```text
   hispark/bed01/#
   ```

6. MQTTX 发布遥测到：

   ```text
   hispark/bed01/telemetry
   ```

7. 遥测 payload：

   ```json
   {
     "deviceId": "bed01",
     "dispplay_mv": 123,
     "display_mv": 123,
     "heartRate": 78,
     "spo2": 98.2,
     "quality": "OK",
     "ts": 1710000000000
   }
   ```

8. 预期结果：电脑端和手机 H5 都显示“已连接”，心电值、心率、血氧、原始遥测消息会立即刷新。

## 患者信息同步验证

1. 保持电脑端和手机端页面同时在线。
2. 在电脑端填写患者信息并点击“上传患者信息”。
3. 预期结果：电脑端当前患者区域更新，手机端无需刷新也更新同一份患者信息，MQTTX 能在 `hispark/bed01/patient` 收到 JSON。
4. 在手机端改一份患者信息并上传。
5. 预期结果：手机端当前患者区域更新，电脑端无需刷新也更新同一份患者信息，MQTTX 能收到新 JSON。

也可以用 MQTTX 直接发布到：

```text
hispark/bed01/patient
```

测试 payload：

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

## WS63 固件验证

1. 在 HiSpark Studio 或 menuconfig 中启用：

   ```text
   ENABLE_MYDEMO_SAMPLE
   SAMPLE_SUPPORT_EMQX_TELEMETRY
   SAMPLE_SUPPORT_EMQX_TELEMETRY_PUBLISHER
   SAMPLE_SUPPORT_SPO2_SENSOR
   ```

2. 如果这个板子同时作为 ECG 星闪服务端接收端，再启用：

   ```text
   SAMPLE_SUPPORT_SLE
   SAMPLE_SUPPORT_AD8232_SLE
   SAMPLE_SUPPORT_AD8232_SLE_SERVER
   ```

3. 在同一个菜单里填写：

   ```text
   WIFI_SSID
   WIFI_PWD
   EMQX_MQTT_USERNAME
   EMQX_MQTT_PASSWORD
   ```

4. 编译并烧录后，串口日志应看到：

   ```text
   [EMQX] wifi ready
   [EMQX] mqtt connected
   [EMQX] subscribed hispark/bed01/patient
   [EMQX] publish {...}
   ```

5. 任一前端或 MQTTX 发布患者信息后，串口日志应看到：

   ```text
   [EMQX] patient name=张三 record=M20260702001 gender=男 age=56 phone=13800000000 note=术后观察
   ```

6. 串口屏控件映射：

   ```text
   t0  心率
   t1  心电实时值
   t2  血氧
   t3  心率
   t4  收缩压
   t5  舒张压
   t6  微循环
   t7  姓名
   t8  病历号
   t9  性别
   t10 年龄
   t11 联系电话
   t12 备注
   ```

7. 预期结果：患者信息写入 `t7` 至 `t12`；已有遥测发布、电脑端、手机端生命体征显示继续正常。
