#include <stdio.h>
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "./app/clock_app.h"
#include "./app/clock_ui.h"
#include "./usart/bsp_debug_usart.h"

/* Keep the ESP-AT driver's millisecond timebase for later migration. */
volatile uint32_t g_system_ms;

static void app_init_task(void *argument)
{
    (void)argument;

    (void)ClockApp_RunStartupStage();
    if (xTaskCreate(ClockApp_TimeTask, "time", 512U, NULL, 1U, NULL) != pdPASS)
    {
        printf("[RTOS] time task creation failed\r\n");
    }
    if (xTaskCreate(ClockApp_NetworkTask, "network", 1024U, NULL, 1U, NULL) != pdPASS)
    {
        printf("[RTOS] network task creation failed\r\n");
    }
    if (xTaskCreate(ClockApp_IndoorTask, "indoor", 512U, NULL, 3U, NULL) != pdPASS)
        printf("[RTOS] indoor task creation failed\r\n");
    vTaskDelete(NULL);
}

int main(void)
{
    /* FreeRTOS requires all implemented priority bits to be preemption bits. */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    Debug_USART_Config();
    printf("[RTOS] scheduler starting\r\n");

    if (!ClockUi_Init() ||
        xTaskCreate(app_init_task, "init", 1024U, NULL, 4U, NULL) != pdPASS)
    {
        printf("[RTOS] task creation failed\r\n");
        for (;;) {}
    }

    vTaskStartScheduler();
    printf("[RTOS] scheduler start failed\r\n");
    for (;;) {}
}

void vApplicationTickHook(void)
{
    ++g_system_ms;
}

void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;
    __disable_irq();
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    __disable_irq();
    for (;;) {}
}
