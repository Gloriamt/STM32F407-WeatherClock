# Weather Clock FreeRTOS application

This directory contains the FreeRTOS version of the STM32F407 weather clock.
Open `Project/RVMDK（uv5）/BH-F407.uvprojx` in Keil and build the `TOUCH`
target. Keil writes local build artifacts under `Output/`.

## Runtime structure

The startup task initializes the display and application state, starts the
worker tasks and then deletes itself. The main page is available immediately;
Wi-Fi connection continues in the background.

- `ui` is the only task that writes to the LCD. Producers update a latest-state
  mailbox and wake the UI through a one-entry queue. The UI redraws only fields
  whose values changed.
- `time` reads the STM32 RTC and publishes clock updates. A valid saved RTC is
  used while offline. SNTP synchronizes the RTC after connection and once per
  hour. After a failed write, the driver attempts to restore the previous RTC
  value. A successful rollback preserves the previous time and validity state;
  a failed rollback clears the validity marker and the UI shows placeholders.
- `indoor` reads the DHT11 every 2 seconds. Its timing-sensitive transaction
  suspends scheduling briefly while leaving interrupts enabled.
- `network` exclusively owns ESP-AT commands. It checks Wi-Fi every 5 seconds,
  treats two consecutive failures as a disconnection, and reconnects in the
  background. The last successful weather data remains visible while offline,
  together with its `HH:MM更新` timestamp. The timestamp is hidden again after
  both current and forecast data have refreshed successfully.

If RTC initialization fails, the clock stays unavailable and initialization is
retried every 60 seconds. Network and weather processing continue during this
condition, and the network time query is deferred until the RTC is available.

## Serial ownership

USART1 PA9/PA10 is the PC debug port at 115200 8-N-1. USART3 PB10/PB11 is the
exclusive ESP-AT channel.

USART3 RX uses an interrupt-driven 1024-byte FreeRTOS queue. The interrupt runs
at priority 5 and stores received bytes in the queue; the network task blocks on
the queue while waiting for ESP responses. No other task sends ESP-AT commands.

## Runtime measurements

The application prints a low-frequency `[METRICS]` report every 60 seconds.
Measurements on the target board on 2026-09-24 produced the following maxima
and minimum remaining stack values:

| Metric | Observed value |
| --- | ---: |
| UI task minimum remaining stack | 940 words |
| Time task minimum remaining stack | 451 words |
| Network task minimum remaining stack | 663 words |
| Indoor task minimum remaining stack | 482 words |
| UI update maximum duration | 11 ms |
| DHT transaction maximum duration | 24 ms |
| Current-weather request maximum duration | 1152 ms |
| Forecast request maximum duration | 1050 ms |
| ESP response overflows | 0 |
| ESP RX queue overflows | 0 |
| ESP dropped bytes | 0 |
| ESP parse errors | 0 |

These values are observations from one hardware test session rather than fixed
limits. They show no current need to replace the ESP byte queue with a ring
buffer or DMA, so the simpler interrupt-and-queue design is retained. Keep the
metrics enabled during longer tests and reconsider the RX design if overflow,
dropped-byte or parser counters increase. Heap usage has not yet been measured.

## Validation status

Hardware validation covers online and offline startup, saved RTC operation,
SNTP synchronization, time/date/weekday display, indoor readings, independent
current and forecast weather updates, runtime disconnection and automatic
reconnection. An injected RTC initialization failure also verified that retries
continue without blocking Wi-Fi or weather processing; the injection was
removed after the test, followed by a normal-path regression test. The periodic
resynchronization path was tested by temporarily shortening its interval to 60
seconds and observing a second successful RTC update. The production interval
was then restored to one hour; a continuous one-hour run has not been performed
solely to verify that final interval. An injected RTC write failure verified the
successful-rollback branch. The branch where both the new write and rollback
fail has not been injected on hardware.

The kernel comes from `WeatherClock-main/third_lib/freertos` (FreeRTOS V10.4.3
LTS Patch 3). `port.c` owns SVC, PendSV and SysTick. The tick hook maintains
`g_system_ms`, including while the scheduler is suspended for the DHT timing
window. The RTOS heap is configured as 32 KiB.
