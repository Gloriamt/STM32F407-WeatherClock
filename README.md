# STM32F407 智能天气时钟

[![Host parser tests](https://github.com/Gloriamt/STM32F407-WeatherClock/actions/workflows/host-parser-tests.yml/badge.svg)](https://github.com/Gloriamt/STM32F407-WeatherClock/actions/workflows/host-parser-tests.yml)

基于 **STM32F407 + FreeRTOS + ESP32-C3** 的天气时钟，显示时间日期、室内温湿度、上海天气及最高／最低温度。本仓库保留裸机版，并提供经过任务重构、故障恢复和实机验证的 FreeRTOS 版。

## 项目来源

本项目是基于“梅花嵌入式”天气时钟参考工程的**复现与二次开发**。参考工程自带 README 指向[《基于 FreeRTOS 的 STM32 智能天气时钟》教学视频](https://www.bilibili.com/video/BV1tfL1zeEQN)。显示布局、天气时钟功能思路和 FreeRTOS 内核来源于参考资料；底层采用野火板级驱动和 ST 标准外设库。

本仓库的工作包括硬件适配、裸机实现、FreeRTOS 迁移和可靠性改进。ST、FreeRTOS、野火及参考工程代码的原有版权声明予以保留，使用或分发时需遵守各自许可。

## 我的改动

- **硬件适配与模块拆分**：将 ESP-AT 迁至独立 USART3，解决与 USART1 调试口的串口争用；修复中文字体索引和天气代码解析，接入多类型天气图标，将应用与页面逻辑移出 `main.c`。
- **任务与显示重构**：从裸机轮询迁移为 UI、时间、网络、室内采集任务；网络任务独占 AT 通道，运行阶段由 UI 任务统一绘图。最新状态邮箱与单元素唤醒队列避免生产任务等待绘图，按字段变化增量刷新。
- **离线与异常恢复**：主页面先显示，Wi-Fi 后台连接；有效 RTC 离线走时、周期校时、写入失败尝试回滚；断网保留历史天气，重连后恢复刷新。实时天气与预报独立提交，单项失败保留对应旧数据。
- **解析与可观测性**：温度使用有符号数并检查格式与范围；区分 AT 错误、超时、响应／接收溢出。增加任务栈、堆、UI/DHT/天气请求耗时指标，以及主机解析测试与 GitHub Actions。

可直接查阅这些改动的记录：[负温度解析 #1](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/1)、[天气独立更新 #2](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/2)、[RTC 可信度 #3](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/3)、[AT 错误处理 #4](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/4)、[周期校时与回滚 #5](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/5)、[后台联网 #6](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/6)、[UI 重构 #7](https://github.com/Gloriamt/STM32F407-WeatherClock/pull/7)。

## 实物效果

![实机照片待补充：此图仅为展示占位，不是开发板运行照片](docs/media/board-placeholder.svg)

| 展示内容 | 当前状态 |
| --- | --- |
| 开发板运行照片 | 待补充：建议展示时间、室内温湿度、上海天气与接线 |
| 断网与重连短视频 | 待补充：建议展示 `-----`、`HH:MM更新` 与重新联网后的恢复 |

补充照片和视频的方法见 [展示素材说明](docs/media/README.md)。

## 两个版本的行为

| 场景 | 裸机版 | FreeRTOS 版 |
| --- | --- | --- |
| 上电 | 启动页查询 Wi-Fi，最长等待约 15 秒，结果停留 3 秒 | 主页面先显示，Wi-Fi 在后台连接 |
| 已连接／断线 | 显示实际 SSID／`-----` | 显示实际 SSID／`-----` |
| 无网络时间 | 本阶段按连接流程显示占位 | 已有可信 RTC 则继续走时，首次未校时则占位 |
| 天气请求部分失败 | 两项请求均成功后刷新卡片 | 实时天气与预报分别更新，失败项保留旧数据 |
| 运行中断网 | 清除室外天气；已同步 RTC 继续走时 | 保留最近天气，有完整更新时间时显示 `HH:MM更新` |
| 再次联网 | 检测连接、配置 SNTP 并获取天气 | 检测连接、配置 SNTP 并获取天气；完整刷新后隐藏更新时间 |
| 周期校时 | 首次成功及重连时校时 | 成功后配置为每小时校时，失败约 10 秒重试 |

Wi-Fi 名称和密码保存在 **ESP32-C3 的 ESP-AT 配置**中；STM32 固件负责查询状态及业务请求，自动连接／重连依赖 ESP 模组配置。天气地点为 `shanghai`，API 响应使用英文，页面通过天气代码显示中文和图标。

## 验证结果

| 场景 | 验证方式与结果 |
| --- | --- |
| 负温度、越界和异常字段 | 主机 C 测试：合法负数正确读取，失败解析不覆盖原值 |
| 实时天气或预报单项失败 | 上板分别模拟两类失败，成功部分继续更新，失败部分保留旧数据 |
| RTC 初始化／写入异常 | 临时故障注入：初始化失败不阻塞天气；写入失败后成功回滚。注入代码已移除 |
| 周期校时 | 临时将间隔缩短至 60 秒，确认第二次校时；正式配置恢复为 1 小时 |
| 断网与重连 | 上板关闭／开启热点，验证 RTC 与室内采集继续、天气保留和重连恢复 |
| 连续运行 | 2026-09-25 完成 24 小时上板运行；最终指标见下表 |

24 小时测试使用野火 STM32F407 霸天虎 V2、4.3 寸 NT35510 LCD、DHT11、ESP32-C3 ESP-AT v4.1.1.0。测试通过 USART1 输出每 60 秒一组的 `[METRICS]`，FreeRTOS 堆配置为 32 KiB。

| 指标 | 24 小时结束时的记录 |
| --- | ---: |
| UI／时间／网络／室内任务最小剩余栈 | 934／445／657／478 words |
| 当前剩余堆／历史最小剩余堆 | 17000／12776 bytes |
| ESP 响应溢出／RX 队列溢出／丢字节／解析错误 | 0／0／0／0 |
| UI／DHT11 最大操作耗时 | 32／24 ms |
| 实时天气／预报最大请求尝试耗时 | 8011／4850 ms |

这些数据是一轮实机观测，受网络和硬件条件影响。`8011 ms` 包含成功或失败的请求尝试，接近 8000 ms 超时配置，不能据此推断成功率；上述 ESP 四项计数也不包含 AT 错误及超时次数。堆最小余量与短期测试相同。测试结束时与手机秒级比较约慢 3 秒，该读数不是 RTC 漂移率测量。

目前仍保留两项验证边界：尚未单独提供带时间戳的正式 1 小时周期校时记录；RTC 新写入与回滚同时失败的分支尚未做实机注入。具体用例和后续复测方法见 [验证说明](docs/validation.md)。

## 快速复现

1. 准备上述硬件，按 [接线与复现指南](docs/reproduction.md) 连接 LCD、DHT11、USART3 和调试口。
2. 通过 ESP-AT 配置可直接访问互联网的 2.4 GHz Wi-Fi，并确认断电后能自动连接。
3. 在需要构建的版本中，将 `User/esp_at/weather_secrets.example.h` 复制为同目录下的 `weather_secrets.h`，填写自己的心知天气私钥。
4. 使用 Keil MDK 打开工程，选择 `TOUCH` target，编译后通过调试器烧录：

| 版本 | Keil 工程入口 |
| --- | --- |
| 裸机 | [bare-metal/Project/RVMDK（uv5）/BH-F407.uvprojx](bare-metal/Project/RVMDK（uv5）/BH-F407.uvprojx) |
| FreeRTOS | [freertos/Project/RVMDK（uv5）/BH-F407.uvprojx](freertos/Project/RVMDK（uv5）/BH-F407.uvprojx) |

5. 以 115200、8-N-1 打开 USART1 调试口，检查联网、校时、天气日志和屏幕。

电脑上可独立运行天气字段解析和 AT 响应捕获测试，不需要开发板或天气私钥。Linux、macOS 或已安装 C 编译器的 WSL 环境执行：

```sh
sh freertos/tests/run_tests.sh
```

用例和编译方式见 [主机测试说明](freertos/tests/README.md)；GitHub Actions 对相关代码变更运行同一脚本。Keil 固件构建和外设时序仍需在开发板验证。

## 代码与文档导航

| 入口 | 内容 |
| --- | --- |
| [裸机实现](bare-metal/README.md) | 单循环应用、启动页、驱动与页面模块 |
| [FreeRTOS 实现](freertos/README.md) | 任务职责、串口所有权和运行指标 |
| [复现指南](docs/reproduction.md) | 硬件清单、接线、Wi-Fi／密钥配置、构建与排查 |
| [验证说明](docs/validation.md) | 主机测试、上板用例、性能条件和已知限制 |
| [完整交接记录](docs/界面复现交接文档.md) | 历史实现过程；第 8 节为当前状态 |
| [资源文件](resource/) | 字库与天气图标原件 |

运行阶段的数据流：RTC 和 DHT11 任务负责采集，网络任务顺序执行 ESP-AT 请求；任务提交最新显示状态，UI 任务统一更新 LCD。USART3 RX 中断写入 1024 字节 FreeRTOS 队列，网络任务等待回复时允许其他任务运行；当前测量没有出现接收溢出，因此保留该实现。

## 凭据与构建产物

本地 `weather_secrets.h`、`Output/`、`Listing/` 和固件构建产物由 `.gitignore` 排除。提交日志、照片或视频前请遮蔽 API 私钥、Wi-Fi 密码及个人信息。更换凭据应在服务端撤销旧密钥；从最新文件删除凭据不会删除 Git 历史中的记录。

2026-10-13 对本地可达的 37 个历史提交进行了文本凭据模式和敏感文件路径检查：密钥定义匹配均为模板占位值，未发现匹配的实际凭据、私钥头文件或 HEX/AXF/BIN 提交。该检查不覆盖图片／二进制中的内容、未获取的远端引用及本地忽略文件，也不构成完整安全审计。

本仓库的改动与验证通过独立分支、PR 和交接记录保留；无需阅读所有 PR 即可按上面的指南复现。
