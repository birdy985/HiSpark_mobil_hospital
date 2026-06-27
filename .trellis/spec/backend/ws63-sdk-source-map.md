# WS63 SDK Source Map

> Where to look before changing WS63/fbb_ws63 embedded code.

---

## Lookup Order

Use this order for every SDK task:

1. Read the local official document for the topic under `fbb_ws63/docs/zh-CN/software/`.
2. Find a working local sample under `fbb_ws63/src/application/samples/` or `fbb_ws63/vendor/`.
3. Inspect the sample `CMakeLists.txt`, `Kconfig`, and entry source file.
4. Inspect SDK headers for exact API signatures and return codes.
5. Inspect target build config, especially `fbb_ws63/src/build/config/target_config/ws63/config.py`, before adding components or macros.
6. Use external sources only when local official docs and local code do not answer the question.

Do not invent API names, CMake variables, task entry patterns, or SLE state transitions.

---

## Official Entry Points

| Question | Primary Source | Notes |
|---|---|---|
| SDK overview and repo layout | `fbb_ws63/README.md` | Defines `docs`, `src`, `tools`, and `vendor`; describes WS63 as a Wi-Fi 6 + NearLink/SLE multimode SDK built on FBB. |
| Software document catalog | `fbb_ws63/docs/zh-CN/software/README.md` | Start here to choose the right official document. |
| Windows build/burn path | `fbb_ws63/tools/HiSparkStudio插件版编译及烧录.md` | Official recommended path in `tools/README.md`. |
| WSL build/burn path | `fbb_ws63/tools/WSL子系统开发环境搭建.md`, `fbb_ws63/tools/WSL子系统编译及烧录.md` | Use when the workflow is WSL + Ubuntu 22.04. |
| SDK build, CMake, menuconfig, new app | `fbb_ws63/docs/zh-CN/software/SDK开发环境搭建 用户指南/SDK开发环境搭建 用户指南.md` | Contains `build.py`, target names, component registration, image outputs, and new app steps. |
| First-use flow | `fbb_ws63/docs/zh-CN/software/快速入门指南/快速入门指南.md` | Use for onboarding, smoke tests, feature map, and high-level development sequence. |

---

## Embedded Application Work

| Question | Source |
|---|---|
| Where to create a new app | `SDK开发环境搭建 用户指南`, section `新建APP` |
| Existing app/sample patterns | `fbb_ws63/src/application/samples/`, especially `mydemo`, `peripheral`, `wifi`, `radar` |
| App entry pattern | Existing samples using `app_run(...)` and task creation via CMSIS or OSAL |
| Component build inclusion | Local `CMakeLists.txt` files and `fbb_ws63/src/build/config/target_config/ws63/config.py` |
| Menuconfig sample toggles | Sample `Kconfig` files and `python3 build.py -c ws63-liteos-app menuconfig` |

For user learning demos, prefer `fbb_ws63/src/application/samples/<topic>/...` unless the work is board-vendor-specific. For custom product applications, follow the official new app flow under `application/ws63/<app>` and register the component in the ws63 target config.

---

## SLE / NearLink Work

| Question | Primary Source | Sample Sources |
|---|---|---|
| SLE development flow | `fbb_ws63/docs/zh-CN/software/软件开发指南/软件开发指南.md`, section `星闪开发流程` | `vendor/HiHope_NearLink_DK_WS63E_V03/demo/sle_throughput` |
| SLE AT commands and manual experiments | `fbb_ws63/docs/zh-CN/software/AT命令 使用指南/AT命令 使用指南.md`, section `SLE` | Use serial AT only for test/control flows, not as application API replacement. |
| High throughput point-to-point | `软件开发指南` SLE `sample示例` | `vendor/HiHope_NearLink_DK_WS63E_V03/demo/sle_throughput` |
| One client to many servers | `软件开发指南` SLE connection notes | `vendor/HiHope_NearLink_DK_WS63E_V03/demo/sle_one_to_many` |
| Gateway aggregation and cloud uplink | SLE + Wi-Fi + MQTT docs | `vendor/developers/demo/sle_gate` |
| Simple board control over SLE | `AT命令 使用指南`, `软件开发指南` | `vendor/HiHope_NearLink_DK_WS63E_V03/demo/sle_led` |
| Wi-Fi/SLE coexistence | `软件开发指南`, section `Wi-Fi&蓝牙共存`; SLE samples | `vendor/HiHope_NearLink_DK_WS63E_V03/demo/sle_wifi_coexist` |
| Other vendor SLE serial patterns | Vendor docs and demos | `vendor/BearPi-Pico_H3863/products/sle_uart`, `vendor/Hqyj_Ws63/Farsight/sle_01_trans_server`, `vendor/Hqyj_Ws63/Farsight/sle_02_trans_client` |

For the planned vital-sign sensing network, use these starting points:

- Throughput/reliability baseline: `sle_throughput`
- Multi-node topology: `sle_one_to_many`
- Patch-to-gateway/cloud architecture: `developers/demo/sle_gate`
- Link-control and reconnect behavior: `软件开发指南` SLE sample notes

Known SLE constraints from official docs/samples:

- WS63V100 supports 8 SLE connections.
- SLE can work concurrently with BLE.
- Current SSAP server registration supports one SSAP server.
- Current SSAP client registration supports one SSAP client.
- On abnormal disconnects or pairing failures, restart advertising/scanning for self-healing.
- Samples use unique server MAC addresses for one-to-many demos; do not reuse addresses across nodes.

---

## Wi-Fi, Networking, and Cloud

| Question | Source |
|---|---|
| Wi-Fi STA, SoftAP, coexistence | `软件开发指南`, section `Wi-Fi 软件开发` |
| lwIP behavior and TCP/IP stack | `fbb_ws63/docs/zh-CN/software/lwIP 开发指南/` |
| MQTT client patterns | `fbb_ws63/docs/zh-CN/software/MQTT 开发指南/`, `src/application/samples/wifi/mqtt_sample`, `vendor/developers/demo/sle_gate` |
| HTTP client | `fbb_ws63/docs/zh-CN/software/HTTP 开发指南/` |
| TLS/DTLS | `fbb_ws63/docs/zh-CN/software/TLS&DTLS 开发指南/` |
| CJSON payload parsing | `fbb_ws63/docs/zh-CN/software/CJSON 开发指南/` |

When SLE data is forwarded to a cloud service, first study `sle_gate` because it already combines SLE client callbacks, message queues, Wi-Fi, MQTT subscribe/publish, and command forwarding.

---

## Driver and Board Work

| Question | Source |
|---|---|
| IO multiplexing | `fbb_ws63/docs/zh-CN/software/IO复用关系.md`, then `设备驱动 开发指南/Pinctrl` |
| GPIO | `设备驱动 开发指南/GPIO` |
| UART | `设备驱动 开发指南/UART`; also `SDK开发环境搭建 用户指南/UART配置方法` |
| SPI | `设备驱动 开发指南/SPI` |
| I2C | `设备驱动 开发指南/I2C` |
| ADC | `设备驱动 开发指南/ADC` |
| DMA | `设备驱动 开发指南/DMA` |
| PWM | `设备驱动 开发指南/PWM` |
| Timer/Systick/TCXO | `设备驱动 开发指南/Timer`, `Systick`, `TCXO` |
| Watchdog | `设备驱动 开发指南/WDT`, `软件开发指南/看门狗` |
| Board-specific wiring | `fbb_ws63/vendor/<board>/doc/` and sample source for that board |

Always verify pinmux and board schematic/doc before choosing pins. UART0 is shared by burning, AT, testsuite, and logs in the default SDK; avoid repurposing it without an explicit board-level decision.

---

## LiteOS, Tasks, Memory, and Debugging

| Question | Source |
|---|---|
| Tasks, semaphores, mutexes, queues | `fbb_ws63/docs/zh-CN/software/LiteOS 开发指南/LiteOS 开发指南.md` |
| Startup and app task creation | `LiteOS 开发指南`, `app_run(...)` samples |
| Crash log interpretation | `fbb_ws63/docs/zh-CN/software/LiteOS 死机问题定位指南/LiteOS 死机问题定位指南.md` |
| Stack overflow and stack sizing | `LiteOS 死机问题定位指南`, `LiteOS 开发指南/栈估算工具使用方法` |
| Memory overwrite/debug | `LiteOS 开发指南/调试案例`, `LiteOS 死机问题定位指南` |
| Deadlock analysis | `LiteOS 开发指南` deadlock sections |
| Watchdog timeout analysis | `LiteOS 死机问题定位指南/看门狗超时` |

FBB-RTOS is single-process and lacks MMU isolation. Treat buffer bounds, task stack size, interrupt workload, and pointer lifetime as system-stability issues, not local bugs.

---

## Storage, Filesystem, Security, and OTA

| Question | Source |
|---|---|
| NV storage | `fbb_ws63/docs/zh-CN/software/NV存储 用户指南/` |
| LittleFS filesystem | `fbb_ws63/docs/zh-CN/software/文件系统 使用指南/` |
| Security APIs | `fbb_ws63/docs/zh-CN/software/安全软件 开发指南/` |
| Boot/flashboot | `Boot API开发参考`, `Boot移植应用 开发指南` |
| Flash partition table | `SDK开发环境搭建 用户指南/Flash分区表配置` and `src/build/config/target_config/ws63/param_sector/param_sector.json` |
| Third-party porting | `fbb_ws63/docs/zh-CN/software/第三方软件 移植指南/` |
| Release behavior | `fbb_ws63/docs/zh-CN/software/release-notes/` |

Do not edit flash partition, boot, NV, or signing configuration as an incidental change.

