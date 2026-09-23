# STM32F407 智能天气时钟

本仓库包含野火 STM32F407 开发板天气时钟的两个实现：

- `bare-metal/`：裸机轮询版本。
- `freertos/`：FreeRTOS 多任务版本，包含运行中 Wi-Fi 断线检测与自动恢复。

两个版本均使用 NT35510 LCD、DHT11、STM32 RTC，以及通过 USART3（PB10/PB11）连接的 ESP32-C3 ESP-AT 固件。USART1（PA9/PA10）保留为电脑调试串口。

## 功能

- 启动页显示 Wi-Fi 连接状态，15 秒超时后进入主页面。
- 连接成功时显示当前 SSID；连接失败或断线时显示 `-----`。
- SNTP 同步时间并显示日期、时间和星期。
- 显示 DHT11 室内温湿度。
- 从心知天气获取上海实时天气、最高/最低温度，并按天气代码切换图标。
- FreeRTOS 版本可在运行中检测断线并自动重新连接。

## 配置天气密钥

天气私钥不会提交到 Git。首次编译前，分别在需要构建的工程中执行：

1. 将 `User/esp_at/weather_secrets.example.h` 复制为 `User/esp_at/weather_secrets.h`。
2. 把 `WEATHER_API_KEY` 的示例值替换成自己的心知天气私钥。

请勿提交 `weather_secrets.h`。已经含有私钥的旧 HEX/AXF 文件也不要公开发布。

## 编译

使用 Keil MDK 打开以下工程文件，选择 `TOUCH` target 后编译：

- 裸机：`bare-metal/Project/RVMDK（uv5）/BH-F407.uvprojx`
- FreeRTOS：`freertos/Project/RVMDK（uv5）/BH-F407.uvprojx`

本仓库不跟踪 `Output` 和 `Listing`。编译后 Keil 会重新生成它们。

## 目录

- `resource/`：字库、天气图标原图及 Image2LCD C 数组。
- `docs/界面复现交接文档.md`：硬件连接、实现过程、验证结果和后续维护说明。

## 验证状态

裸机版和 FreeRTOS 版均已在实机完成全流程测试，包括 Wi-Fi 成功/失败启动路径、时间日期星期、室内温湿度、天气更新，以及 FreeRTOS 版的运行中断线与自动重连。

## 第三方代码

工程包含 ST 标准外设库、FreeRTOS 和野火板级驱动。相关文件继续受其原始版权声明与许可条款约束。
