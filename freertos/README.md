# Weather Clock FreeRTOS migration: runtime Wi-Fi recovery stage

This is an independent copy of the validated bare-metal project. Open
`Project/RVMDK（uv5）/BH-F407.uvprojx` in Keil and build the `TOUCH` target.
Keil generates the build output locally under `Output/`.

The scheduler smoke test and both startup Wi-Fi paths have passed on hardware.
The `init` task initializes the LCD, displays the startup page, waits up to 15
seconds for Wi-Fi, shows the result for 3 seconds and opens the main page.

After startup, the `init` task creates a separate `time` task and deletes
itself. The time task initializes the F407 RTC and posts clock updates to the
UI queue after network time becomes available. With no network time, the page
keeps the time placeholder.

A dedicated `ui` task is the only task that performs incremental LCD updates.
The `indoor` task reads DHT11 every 2 seconds and posts copied values to the UI
queue. Its timing-sensitive transaction suspends task scheduling for about 20
ms but leaves interrupts enabled.

USART3 reception now follows the reference project's interrupt-driven design.
Its RX interrupt stores bytes in a 1024-byte FreeRTOS queue at interrupt
priority 5. The single `network` task owns all ESP-AT commands and blocks on
that queue while waiting for replies, allowing the clock, UI and indoor tasks
to keep running during HTTP requests. It performs SNTP synchronization and
requests current weather plus the daily high/low every 60 seconds. Weather is
copied to the UI queue.

The network task checks Wi-Fi every 5 seconds. Two consecutive failed checks
mark it disconnected, post `-----` for the SSID and clear outdoor weather.
The F407 RTC and indoor readings continue. The network task also exists after
an initial 15-second startup failure, so a later connection is detected. On
reconnection it posts the current SSID, configures SNTP again, synchronizes the
RTC and immediately requests weather.

USART1 PA9/PA10 remains the PC debug port at 115200 8-N-1. USART3 PB10/PB11
remains the exclusive ESP-AT channel. Only one task currently uses ESP-AT.

The kernel is copied from `WeatherClock-main/third_lib/freertos` (FreeRTOS
V10.4.3 LTS Patch 3). `port.c` owns SVC, PendSV and SysTick. The tick hook
maintains `g_system_ms` for the later driver migration. The RTOS heap starts
at 32 KiB; measure stack and heap before adding business tasks.

The complete flow has passed hardware testing: scheduler,
both startup Wi-Fi paths, time/date/weekday, indoor readings, weather updates,
runtime disconnection and automatic reconnection.
