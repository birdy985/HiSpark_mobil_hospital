(function () {
  const config = {
    host: "q67a1139.ala.cn-shenzhen.emqxsl.cn",
    websocketPort: 8084,
    path: "/mqtt",
    username: "CHANGE_ME",
    password: "CHANGE_ME",
    telemetryTopic: "hispark/bed01/telemetry",
    patientTopic: "hispark/bed01/patient",
    clientPrefix: "hispark_dashboard_"
  };

  const urlParams = new URLSearchParams(window.location.search);
  const isMobilePage = document.body.dataset.view === "mobile";
  config.username = urlParams.get("user") || config.username;
  config.password = urlParams.get("pass") || config.password;

  const state = {
    client: null,
    connected: false,
    lastPayload: null,
    lastMessageAt: null,
    patientSeq: 0,
    wave: [],
    maxWavePoints: 180
  };

  const elements = {};

  function byId(id) {
    return document.getElementById(id);
  }

  function bindElements() {
    [
      "statusPill", "statusText", "topicText", "ecgValue", "heartRateValue", "spo2Value",
      "lastUpdateText", "deviceIdText", "messageCountText", "qualityText", "rawPayload",
      "waveCanvas", "mobileWaveCanvas", "patientForm", "patientNameInput", "patientRecordInput",
      "patientGenderInput", "patientAgeInput", "patientPhoneInput", "patientNoteInput",
      "patientStatusText", "patientNameText", "patientRecordText", "patientGenderText",
      "patientAgeText", "patientPhoneText", "patientNoteText", "patientSourceText",
      "patientUpdatedText"
    ].forEach((id) => {
      elements[id] = byId(id);
    });
  }

  function setStatus(status, text) {
    if (!elements.statusPill || !elements.statusText) {
      return;
    }
    elements.statusPill.classList.remove("connected", "error", "offline");
    if (status) {
      elements.statusPill.classList.add(status);
    }
    elements.statusText.textContent = text;
  }

  function setPatientStatus(text) {
    updateText(elements.patientStatusText, text);
  }

  function mqttUrl() {
    return `wss://${config.host}:${config.websocketPort}${config.path}`;
  }

  function safeNumber(value) {
    if (value === null || value === undefined || value === "") {
      return null;
    }
    const num = Number(value);
    return Number.isFinite(num) ? num : null;
  }

  function formatValue(value, decimals) {
    const num = safeNumber(value);
    if (num === null) {
      return "--";
    }
    return decimals > 0 ? num.toFixed(decimals) : String(Math.round(num));
  }

  function pickTelemetry(payload) {
    const displayMv = payload.dispplay_mv ?? payload.display_mv ?? payload.disp_mv ?? payload.ecg_mv;
    const heartRate = payload.heartRate ?? payload.heart_rate ?? payload.hr_bpm ?? payload.bpm;
    const spo2 = payload.spo2 ?? payload.spo2Percent ?? payload.spo2_percent;
    const spo2X10 = payload.spo2_x10;
    return {
      deviceId: payload.deviceId ?? payload.device_id ?? payload.bedNo ?? "bed01",
      displayMv,
      heartRate,
      spo2: spo2 !== undefined ? spo2 : (spo2X10 !== undefined ? Number(spo2X10) / 10 : undefined),
      quality: payload.quality ?? payload.ecgQuality ?? payload.spo2_quality ?? payload.state ?? "--",
      ts: payload.ts ?? payload.timestamp_ms ?? payload.t_ms ?? Date.now()
    };
  }

  function updateText(el, value) {
    if (el) {
      el.textContent = value;
    }
  }

  function cleanText(value, maxLen) {
    return String(value || "")
      .replace(/[\u0000-\u001f\u007f]/g, " ")
      .replace(/["\\]/g, " ")
      .replace(/\s+/g, " ")
      .trim()
      .slice(0, maxLen);
  }

  function pushWave(value) {
    const num = safeNumber(value);
    if (num === null) {
      return;
    }
    state.wave.push(num);
    if (state.wave.length > state.maxWavePoints) {
      state.wave.splice(0, state.wave.length - state.maxWavePoints);
    }
    drawWave(elements.waveCanvas);
    drawWave(elements.mobileWaveCanvas);
  }

  function drawWave(canvas) {
    if (!canvas) {
      return;
    }
    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(1, Math.floor(rect.width * dpr));
    const height = Math.max(1, Math.floor(rect.height * dpr));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }

    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, width, height);
    ctx.fillStyle = "#10202f";
    ctx.fillRect(0, 0, width, height);

    ctx.strokeStyle = "rgba(255,255,255,0.08)";
    ctx.lineWidth = 1 * dpr;
    for (let i = 1; i < 5; i += 1) {
      const y = (height / 5) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }

    const values = state.wave;
    if (values.length < 2) {
      ctx.fillStyle = "rgba(255,255,255,0.45)";
      ctx.font = `${12 * dpr}px Arial`;
      ctx.fillText("等待 MQTT 遥测数据", 14 * dpr, 28 * dpr);
      return;
    }

    let min = Math.min(...values);
    let max = Math.max(...values);
    if (min === max) {
      min -= 1;
      max += 1;
    }
    const pad = Math.max((max - min) * 0.18, 1);
    min -= pad;
    max += pad;

    ctx.strokeStyle = "#35d0ba";
    ctx.lineWidth = 2 * dpr;
    ctx.beginPath();
    values.forEach((value, index) => {
      const x = (index / (state.maxWavePoints - 1)) * width;
      const y = height - ((value - min) / (max - min)) * height;
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.stroke();
  }

  function updateDashboard(payload, rawText) {
    const telemetry = pickTelemetry(payload);
    state.lastPayload = payload;
    state.lastMessageAt = new Date();

    updateText(elements.ecgValue, formatValue(telemetry.displayMv, 0));
    updateText(elements.heartRateValue, formatValue(telemetry.heartRate, 0));
    updateText(elements.spo2Value, formatValue(telemetry.spo2, 1));
    updateText(elements.lastUpdateText, state.lastMessageAt.toLocaleString());
    updateText(elements.deviceIdText, telemetry.deviceId);
    updateText(elements.qualityText, telemetry.quality);
    updateText(elements.messageCountText, String((Number(elements.messageCountText?.textContent) || 0) + 1));
    updateText(elements.rawPayload, rawText || JSON.stringify(payload, null, 2));

    pushWave(telemetry.displayMv);
  }

  function readPatientForm() {
    return {
      name: cleanText(elements.patientNameInput?.value, 20),
      recordNo: cleanText(elements.patientRecordInput?.value, 30),
      gender: cleanText(elements.patientGenderInput?.value, 8),
      age: safeNumber(elements.patientAgeInput?.value),
      phone: cleanText(elements.patientPhoneInput?.value, 24),
      note: cleanText(elements.patientNoteInput?.value, 60)
    };
  }

  function validatePatient(patient) {
    if (!patient.name) {
      return "请填写病人姓名";
    }
    if (!patient.recordNo) {
      return "请填写病历号";
    }
    if (!["男", "女", "其他"].includes(patient.gender)) {
      return "请选择性别";
    }
    if (patient.age === null || patient.age < 0 || patient.age > 130) {
      return "请填写有效年龄";
    }
    if (patient.phone && !/^[0-9+\-()\s]{3,24}$/.test(patient.phone)) {
      return "联系电话格式不正确";
    }
    return null;
  }

  function updatePatientView(payload) {
    const patient = payload.patient || {};
    const age = safeNumber(patient.age);
    const updatedAt = payload.ts ? new Date(Number(payload.ts)) : new Date();

    updateText(elements.patientNameText, patient.name || "未登记");
    updateText(elements.patientRecordText, patient.recordNo ? `病历号 ${patient.recordNo}` : "病历号 --");
    updateText(elements.patientGenderText, patient.gender || "--");
    updateText(elements.patientAgeText, age === null ? "--" : `${Math.round(age)} 岁`);
    updateText(elements.patientPhoneText, patient.phone || "--");
    updateText(elements.patientNoteText, patient.note || "--");
    updateText(elements.patientSourceText, payload.source || "--");
    updateText(elements.patientUpdatedText, updatedAt.toLocaleString());
    setPatientStatus("已同步最新患者信息");
  }

  function handlePatientInfo(payload) {
    if (!payload || payload.type !== "patientInfo" || !payload.patient) {
      return;
    }
    updatePatientView(payload);
  }

  function publishPatientInfo(event) {
    event.preventDefault();
    const patient = readPatientForm();
    const error = validatePatient(patient);
    if (error) {
      setPatientStatus(error);
      return;
    }
    if (!state.connected || !state.client) {
      setPatientStatus("MQTT 未连接，无法上传");
      return;
    }

    const payload = {
      type: "patientInfo",
      deviceId: "bed01",
      source: isMobilePage ? "mobile" : "pc",
      seq: ++state.patientSeq,
      ts: Date.now(),
      patient: {
        name: patient.name,
        recordNo: patient.recordNo,
        gender: patient.gender,
        age: Math.round(patient.age),
        phone: patient.phone,
        note: patient.note
      }
    };

    state.client.publish(config.patientTopic, JSON.stringify(payload), { qos: 0, retain: false }, (publishError) => {
      if (publishError) {
        setPatientStatus(publishError.message || "患者信息上传失败");
        return;
      }
      setPatientStatus("已上传，等待同步回显");
    });
  }

  function bindPatientForm() {
    if (elements.patientForm) {
      elements.patientForm.addEventListener("submit", publishPatientInfo);
    }
  }

  function handleMessage(topic, message) {
    const rawText = new TextDecoder("utf-8").decode(message);
    try {
      const payload = JSON.parse(rawText);
      if (topic === config.patientTopic) {
        handlePatientInfo(payload);
        return;
      }
      updateDashboard(payload, rawText);
    } catch (error) {
      if (topic === config.telemetryTopic) {
        updateText(elements.rawPayload, rawText);
      }
      setStatus("error", "消息不是 JSON");
    }
  }

  function subscribeTopics(client) {
    client.subscribe([config.telemetryTopic, config.patientTopic], { qos: 0 }, (error) => {
      if (error) {
        setStatus("error", "订阅失败");
        updateText(elements.rawPayload, error.message || String(error));
        return;
      }
      setPatientStatus("已订阅患者信息");
    });
  }

  function connect() {
    if (config.username === "CHANGE_ME" || config.password === "CHANGE_ME") {
      setStatus("error", "请配置 MQTT 账号");
      setPatientStatus("请配置 MQTT 账号");
      updateText(elements.rawPayload, "在 dashboard.js 顶部配置账号，或使用 ?user=账号&pass=密码 打开页面。");
      return;
    }
    if (!window.mqtt) {
      setStatus("error", "MQTT.js 未加载");
      setPatientStatus("MQTT.js 未加载");
      return;
    }
    setStatus(null, "连接中");
    updateText(elements.topicText, `${config.telemetryTopic} / ${config.patientTopic}`);

    const clientId = `${config.clientPrefix}${isMobilePage ? "mobile" : "pc"}_${Math.random().toString(16).slice(2, 8)}`;
    const client = window.mqtt.connect(mqttUrl(), {
      clientId,
      username: config.username,
      password: config.password,
      clean: true,
      connectTimeout: 10000,
      keepalive: 60,
      reconnectPeriod: 3000,
      protocolVersion: 4
    });
    state.client = client;

    client.on("connect", () => {
      state.connected = true;
      setStatus("connected", "已连接");
      subscribeTopics(client);
    });

    client.on("reconnect", () => setStatus(null, "重连中"));
    client.on("close", () => {
      state.connected = false;
      setStatus("offline", "已断开");
    });
    client.on("error", (error) => {
      setStatus("error", "连接错误");
      setPatientStatus("连接错误");
      updateText(elements.rawPayload, error.message || String(error));
    });
    client.on("message", handleMessage);
  }

  window.addEventListener("resize", () => {
    drawWave(elements.waveCanvas);
    drawWave(elements.mobileWaveCanvas);
  });

  window.addEventListener("DOMContentLoaded", () => {
    bindElements();
    bindPatientForm();
    updateText(elements.topicText, `${config.telemetryTopic} / ${config.patientTopic}`);
    drawWave(elements.waveCanvas);
    drawWave(elements.mobileWaveCanvas);
    connect();
  });
}());
