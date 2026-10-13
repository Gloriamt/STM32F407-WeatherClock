# 接线与复现指南

本指南对应仓库中的 `bare-metal/` 和 `freertos/`，建议先使用 FreeRTOS 版完成下列步骤。

## 1. 硬件与软件

| 项目 | 已验证配置 |
| --- | --- |
| 主控 | 野火 STM32F407 霸天虎 V2 |
| 显示屏 | 4.3 寸 NT35510 电容屏，纵向逻辑分辨率 480×800，使用开发板配套接口 |
| 室内传感器 | DHT11，3.3 V、PE3 数据线、GND，板上有 10 kΩ 上拉 |
| 网络模组 | ESP32-C3，ESP-AT v4.1.1.0，AT 通道使用 GPIO6/7 |
| 调试接口 | 开发板 USART1／板载 CH340，串口助手设置为 115200 8-N-1 |
| 构建工具 | Keil MDK，当前工程配置为 ARM Compiler 5.06 update 6（build 750），使用 ST 标准外设库 |
| 下载工具 | 与开发板兼容的 SWD 调试器 |
| 网络 | 可直接联网的 2.4 GHz Wi-Fi；本指南不覆盖网页或企业账号认证 |
| 天气服务 | 心知天气私钥，具有上海实时天气及当天预报访问权限 |

其他模组、LCD 或开发板需要相应调整驱动和引脚；本指南中的数值仅对应上表硬件。

## 2. 串口与 DHT11 接线

| 发端 | 收端 | 用途 |
| --- | --- | --- |
| F407 PB10／USART3_TX | ESP32-C3 GPIO6／UART1_RX | 发送 AT 命令 |
| ESP32-C3 GPIO7／UART1_TX | F407 PB11／USART3_RX | 返回 AT 响应 |
| F407 GND | ESP32-C3 GND | 共地 |
| F407 PA9／USART1_TX、PA10／USART1_RX | 板载 CH340／电脑 | 调试日志 |
| DHT11 数据线 | F407 PE3 | 温湿度采集 |

LCD 使用开发板配套屏幕接口，DHT11 使用 J5 的 3.3 V、PE3 和 GND。按模组板的供电说明给 ESP32-C3 供电，串口信号使用 3.3 V 电平。

不要让 ESP 与 CH340 共用 F407 USART1。当前 ESP-AT 固件的 USB 和 UART0 是下载／启动日志通道，不能代替这里的 UART1 AT 通道。若更换 ESP-AT 固件，需要先确认其 UART 引脚配置。

## 3. 配置 ESP32-C3 Wi-Fi

STM32 应用没有硬编码 Wi-Fi 名称和密码，也不通过调试口转发手工 AT 命令。请使用先前的串口透传固件，或暂时断开 F407 与 ESP 的串口连接，改用 USB-UART 接 ESP 的 AT 通道进行配置。

串口助手使用 115200 8-N-1，每条命令以 CRLF 结束，逐条发送并等待回复：

```text
AT
AT+SYSSTORE=1
AT+CWMODE=1
AT+CWJAP="YOUR_WIFI_SSID","YOUR_WIFI_PASSWORD"
AT+CWAUTOCONN=1
AT+CWSTATE?
```

将占位值替换为本地网络信息；普通无密码网络使用空密码 `""`。`AT+CWSTATE?` 返回状态 `2` 和 SSID 表示已连接并取得 IP。使用 `AT+RST` 或断电重启 ESP 后再次查询，确认配置已保存且能够自动连接。不同 AT 固件的命令支持情况以其对应版本文档为准。

配置完成后恢复 F407 与 ESP 的接线，运行天气时钟固件。应用通过 `AT+CWSTATE?` 观察连接状态；ESP 的自动连接配置承担实际连接动作。只有连接 Wi-Fi 不代表天气 API 或 NTP 可达，还要检查后续校时和天气日志。

## 4. 配置天气私钥

在需要构建的工程目录执行下面的操作：

1. 将 `User/esp_at/weather_secrets.example.h` 复制为同目录下的 `weather_secrets.h`。
2. 把 `WEATHER_API_KEY` 的占位值替换为自己的心知天气**私钥**。
3. 确认服务权限允许访问 `weather/now.json` 和 `weather/daily.json` 的 `shanghai` 地点。

两个版本分别使用各自的本地头文件。这里使用英文 API 返回值避免串口中文编码问题；页面文字来自本地字体和天气代码映射。请勿将本地私钥头文件或含有私钥的 HEX/AXF 公开上传。

## 5. 构建与烧录

在 Keil MDK 打开以下工程之一，选择 `TOUCH` target：

- [FreeRTOS 工程](../freertos/Project/RVMDK（uv5）/BH-F407.uvprojx)
- [裸机工程](../bare-metal/Project/RVMDK（uv5）/BH-F407.uvprojx)

确认工程要求的编译器可用；使用仅包含 ARM Compiler 6 的 MDK 安装时，需先配置 Compiler 5，不能把已有测试结果直接视为 Compiler 6 的兼容性验证。编译后选择开发板对应的调试器和 STM32F407 Flash 算法，通过 Download 烧录。输出生成在版本目录的 `Output/`，不随 Git 分发。若使用离线烧录工具，应选本次编译生成的 HEX，而不是历史固件归档。

打开调试串口后复位 F407，FreeRTOS 版预期看到：

```text
[RTOS] scheduler starting
[RTOS] main page ready; Wi-Fi connecting in background
[WIFI] connected
[SNTP] configured
[SNTP] RTC updated
[WEATHER] current temperature=...
[WEATHER] forecast high=... low=...
```

后续约每 60 秒输出栈、耗时、ESP 接收诊断和堆余量。主页面可先于联网完成显示；从未取得可信 RTC 时间时先显示占位。已有有效 RTC 时，即使没有 Wi-Fi 也可以显示保存的时间。

## 6. 电脑可独立运行的测试

主机测试只编译两组纯 C 模块，不依赖 STM32、FreeRTOS、密钥或网络：

```sh
sh freertos/tests/run_tests.sh
```

需要 POSIX shell 和 `cc`，可在 Linux、macOS 或配置好 C 编译器的 WSL 使用。设置 `CC=clang` 可选择其他编译器。预期输出：

```text
weather_parser_test: PASS
at_response_test: PASS
```

仓库的 [GitHub Actions](../.github/workflows/host-parser-tests.yml) 对解析代码或测试的变更运行同样脚本。Windows MSVC 的两组用例也曾在 `/std:c11 /W4 /WX` 配置下通过；完整用例范围见 [主机测试 README](../freertos/tests/README.md)。

## 7. 常见排查

| 现象 | 首先检查 |
| --- | --- |
| `weather_secrets.h` 找不到 | 是否在正在构建的版本中复制了模板 |
| ESP 不回复 AT | 接的是 AT UART 而不是日志口；TX/RX 是否交叉、共地，波特率是否正确 |
| `waiting for download` | ESP 仍在下载模式，检查 BOOT 和复位状态 |
| SSID 持续显示 `-----` | ESP 的保存配置和自动连接；网络是否支持 2.4 GHz |
| Wi-Fi 成功但校时／天气失败 | NTP、DNS、HTTPS 是否可达，天气私钥和接口权限是否正确 |
| 时间占位 | 首次没有可信网络时间，或 RTC 初始化／恢复失败；结合 `[RTC]` 日志判断 |
| 天气“更新”提示未消失 | 实时天气和预报是否在同一轮完整刷新，时间状态是否有效 |
| 页面中文字破碎 | 字库索引、单字字节数与绘制尺寸是否匹配 |

进入新一轮功能修改前，按 [验证说明](validation.md) 做回归，并在独立分支保留改动及测量结果。
