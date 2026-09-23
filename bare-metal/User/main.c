#include "stm32f4xx.h"
#include "./app/clock_app.h"

/* Shared millisecond tick for the bare-metal application and ESP-AT driver. */
volatile uint32_t g_system_ms;

int main(void)
{
    ClockApp_Run();
    return 0;
}
