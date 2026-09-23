# F407 Weather Clock (bare metal)

This Keil project targets the Wildfire STM32F407 board with the NT35510 LCD.
It uses ESP-AT over USART3 (PB10/PB11), a DHT11 indoor sensor, and the F407 RTC.
The PC debug serial port remains USART1.

## Source layout

- `User/main.c`: application entry point and shared millisecond tick.
- `User/app/clock_app.c`: Wi-Fi startup, SNTP synchronization, and periodic weather, indoor, and RTC updates.
- `User/page/startup_page.c`: startup screen and Wi-Fi result.
- `User/page/main_page.c`: dashboard drawing and weather-code-to-icon mapping.
- `User/esp_at/`: ESP-AT transport, time, and weather requests.
- `User/rtc/`, `User/dht11/`, `User/lcd/`, `User/font/`, `User/image/`: device drivers and display resources.

This stage intentionally uses a bare-metal loop. The reference project's
FreeRTOS tasks and queue-based UI are not part of this firmware.

## Build

Open `Project/RVMDK（uv5）/BH-F407.uvprojx` in Keil, build, and download to
the board. The project file includes the application and both page modules.

Weather icon arrays under `User/image/weather/` are resized from the supplied
Image2LCD C arrays by `tools/convert_weather_icons.ps1` to fit the STM32F407's
1 MB Flash.
