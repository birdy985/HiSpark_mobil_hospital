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
    patientSeq: 0,
    messageCount: 0
  };

  const elements = {};

  function byId(id) {
    return document.getElementById(id);
  }

  function bindElements() {
    [
      "statusPill", "statusText", "heartRateValue", "spo2Value", "microcirculationValue",
      "systolicBpValue", "diastolicBpValue", "lastUpdateText", "deviceIdText", "messageCountText",
      "patientForm", "patientNameInput", "patientRecordInput", "patientGenderInput", "patientAgeInput",
      "patientPhoneInput", "patientNoteInput", "patientStatusText", "patientNameText", "patientRecordText",
      "patientGenderText", "patientAgeText", "patientPhoneText", "patientNoteText", "patientSourceText",
      "patientUpdatedText"
    ].forEach((id) => {
      elements[id] = byId(id);
    });
  }

  function updateText(el, value) {
    if (el) {
      el.textContent = value;
    }
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

  function firstDefined(payload, keys) {
    for (const key of keys) {
      if (payload[key] !== undefined && payload[key] !== null) {
        return payload[key];
      }
    }
    return undefined;
  }

  function readFlag(payload, keys) {
    for (const key of keys) {
      const value = payload[key];
      if (typeof value === "boolean") {
        return value;
      }
      if (value === "true") {
        return true;
      }
      if (value === "false") {
        return false;
      }
    }
    return undefined;
  }

  function metricValue(payload, valueKeys, validKeys) {
    const valid = readFlag(payload, validKeys);
    if (valid === false) {
      return null;
    }
    return firstDefined(payload, valueKeys);
  }

  function formatValue(value, decimals) {
    const num = safeNumber(value);
    if (num === null) {
      return "--";
    }
    return decimals > 0 ? num.toFixed(decimals) : String(Math.round(num));
  }

  function pickTelemetry(payload) {
    const spo2Direct = metricValue(payload, ["spo2", "spo2Percent", "spo2_percent"], ["spo2Valid", "spo2_valid"]);
    const spo2X10 = metricValue(payload, ["spo2_x10"], ["spo2Valid", "spo2_valid"]);
    const spo2 = spo2Direct !== undefined && spo2Direct !== null ? spo2Direct :
      (spo2X10 !== undefined && spo2X10 !== null ? Number(spo2X10) / 10 : undefined);

    return {
      deviceId: payload.deviceId ?? payload.device_id ?? payload.bedNo ?? "bed01",
      heartRate: metricValue(payload, ["heartRate", "heart_rate", "hr_bpm", "bpm"], ["heartRateValid", "heart_rate_valid", "hr_valid"]),
      spo2,
      microcirculation: metricValue(payload,
        ["microcirculation", "micro_circulation", "microcirculationIndex"],
        ["microcirculationValid", "microcirculation_valid"]),
      systolicBp: metricValue(payload,
        ["systolicBp", "systolic_bp", "systolicPressure", "systolic_pressure"],
        ["bpValid", "bp_valid", "systolicBpValid", "systolic_bp_valid"]),
      diastolicBp: metricValue(payload,
        ["diastolicBp", "diastolic_bp", "diastolicPressure", "diastolic_pressure"],
        ["bpValid", "bp_valid", "diastolicBpValid", "diastolic_bp_valid"]),
      ts: payload.ts ?? payload.timestamp_ms ?? payload.t_ms ?? Date.now()
    };
  }

  function updateDashboard(payload) {
    const telemetry = pickTelemetry(payload);
    const updatedAt = telemetry.ts ? new Date(Number(telemetry.ts)) : new Date();

    state.messageCount += 1;
    updateText(elements.heartRateValue, formatValue(telemetry.heartRate, 0));
    updateText(elements.spo2Value, formatValue(telemetry.spo2, Number(telemetry.spo2) % 1 === 0 ? 0 : 1));
    updateText(elements.microcirculationValue, formatValue(telemetry.microcirculation, 0));
    updateText(elements.systolicBpValue, formatValue(telemetry.systolicBp, 0));
    updateText(elements.diastolicBpValue, formatValue(telemetry.diastolicBp, 0));
    updateText(elements.deviceIdText, telemetry.deviceId);
    updateText(elements.lastUpdateText, updatedAt.toLocaleString());
    updateText(elements.messageCountText, String(state.messageCount));
  }

  function cleanText(value, maxLen) {
    return String(value || "")
      .replace(/[\u0000-\u001f\u007f]/g, " ")
      .replace(/["\\]/g, " ")
      .replace(/\s+/g, " ")
      .trim()
      .slice(0, maxLen);
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
      if (topic === config.telemetryTopic) {
        updateDashboard(payload);
      }
    } catch (error) {
      setStatus("error", "消息格式错误");
    }
  }

  function subscribeTopics(client) {
    client.subscribe([config.telemetryTopic, config.patientTopic], { qos: 0 }, (error) => {
      if (error) {
        setStatus("error", "订阅失败");
        setPatientStatus(error.message || "订阅失败");
        return;
      }
      setPatientStatus("已连接患者信息通道");
    });
  }

  function connect() {
    if (config.username === "CHANGE_ME" || config.password === "CHANGE_ME") {
      setStatus("error", "请配置 MQTT 账号");
      setPatientStatus("请配置 MQTT 账号");
      return;
    }
    if (!window.mqtt) {
      setStatus("error", "MQTT.js 未加载");
      setPatientStatus("MQTT.js 未加载");
      return;
    }
    setStatus(null, "连接中");

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
      setPatientStatus(error.message || "连接错误");
    });
    client.on("message", handleMessage);
  }

  window.addEventListener("DOMContentLoaded", () => {
    bindElements();
    bindPatientForm();
    connect();
  });
}());
