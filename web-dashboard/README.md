# HiSpark EMQX Dashboard

静态电脑端和手机 H5 页面，用于订阅 EMQX `hispark/bed01/telemetry` 并实时显示：

- `dispplay_mv` / `display_mv`
- `heartRate` / `bpm` / `hr_bpm`
- `spo2` / `spo2_x10`

## 页面

- 电脑端：`index.html`
- 手机端 H5：`mobile.html`

## MQTT 连接

配置集中在 `dashboard.js` 顶部：

```js
host: "q67a1139.ala.cn-shenzhen.emqxsl.cn"
websocketPort: 8084
path: "/mqtt"
topic: "hispark/bed01/telemetry"
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

5. MQTTX 发布到：

   ```text
   hispark/bed01/telemetry
   ```

6. 测试 payload：

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

7. 预期结果：电脑端和手机 H5 都显示“已连接”，心电值、心率、血氧、原始消息会立即刷新。

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
   [EMQX] publish {...}
   ```

5. MQTTX 订阅：

   ```text
   hispark/bed01/#
   ```

6. 预期结果：MQTTX、电脑页面、手机 H5 都能收到 WS63 每秒发布的 `hispark/bed01/telemetry` 数据。
