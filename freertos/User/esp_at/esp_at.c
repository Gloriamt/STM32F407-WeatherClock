#include "esp_at.h"
#include "weather_secrets.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern volatile uint32_t g_system_ms;

#define ESP_USART              USART3
#define ESP_USART_CLK          RCC_APB1Periph_USART3
#define ESP_USART_TX_PORT      GPIOB
#define ESP_USART_RX_PORT      GPIOB
#define ESP_USART_TX_PIN       GPIO_Pin_10
#define ESP_USART_RX_PIN       GPIO_Pin_11
#define ESP_USART_TX_SOURCE    GPIO_PinSource10
#define ESP_USART_RX_SOURCE    GPIO_PinSource11
#define ESP_RX_QUEUE_LENGTH    1024U

static char sntp_last_response[256];
static QueueHandle_t esp_rx_queue;

static void esp_uart_config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(ESP_USART_CLK, ENABLE);
    GPIO_PinAFConfig(GPIOB, ESP_USART_TX_SOURCE, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, ESP_USART_RX_SOURCE, GPIO_AF_USART3);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = ESP_USART_TX_PIN | ESP_USART_RX_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    USART_StructInit(&usart);
    usart.USART_BaudRate = 115200;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ESP_USART, &usart);

    nvic.NVIC_IRQChannel = USART3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 5U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    NVIC_SetPriority(USART3_IRQn, 5U);
    USART_ITConfig(ESP_USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(ESP_USART, ENABLE);
}

static void uart_puts(const char *s);

static void uart_begin_command(const char *command)
{
    USART_ITConfig(ESP_USART, USART_IT_RXNE, DISABLE);
    (void)xQueueReset(esp_rx_queue);
    while (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET)
        (void)USART_ReceiveData(ESP_USART);
    USART_ITConfig(ESP_USART, USART_IT_RXNE, ENABLE);
    uart_puts(command);
}

static uint8_t uart_wait_response(char *response, uint16_t response_size,
                                  uint32_t timeout_ms)
{
    uint16_t position = 0U;
    uint32_t start = g_system_ms;
    uint32_t elapsed;
    uint8_t byte;

    response[0] = 0;
    while ((elapsed = g_system_ms - start) < timeout_ms)
    {
        if (xQueueReceive(esp_rx_queue, &byte,
                          pdMS_TO_TICKS(timeout_ms - elapsed)) != pdPASS)
            break;

        if (position < response_size - 1U)
        {
            response[position++] = (char)byte;
            response[position] = 0;
        }
        if (strstr(response, "\r\nOK\r\n") != NULL)
            return 1U;
        if (strstr(response, "\r\nERROR\r\n") != NULL)
            return 0U;
    }
    return 0U;
}

uint8_t EspAt_RequestWeather(esp_weather_t *w)
{
    char cmd[260], buf[900];
    sprintf(cmd, "AT+HTTPCLIENT=2,1,\"https://api.seniverse.com/v3/weather/now.json?key=%s&location=shanghai&language=en&unit=c\",,,2\r\n", WEATHER_API_KEY);
    uart_begin_command(cmd);
    if (uart_wait_response(buf, sizeof(buf), 8000U)) {
        char *p = strstr(buf, "\"temperature\":\"");
        char *c = strstr(buf, "\"code\":\"");
        if (!p || !c) return 0;
        w->temperature = (uint8_t)atoi(p + (sizeof("\"temperature\":\"") - 1U));
        w->code = (uint8_t)atoi(c + (sizeof("\"code\":\"") - 1U));
        return 1;
    }
    return 0;
}

uint8_t EspAt_RequestForecast(esp_weather_t *w)
{
    char cmd[280], buf[900];
    sprintf(cmd,"AT+HTTPCLIENT=2,1,\"https://api.seniverse.com/v3/weather/daily.json?key=%s&location=shanghai&language=en&unit=c&start=0&days=1\",,,2\r\n",WEATHER_API_KEY);
    uart_begin_command(cmd);
    if (uart_wait_response(buf, sizeof(buf), 8000U)) {
        char *h = strstr(buf, "\"high\":\"");
        char *l = strstr(buf, "\"low\":\"");
        if (!h || !l) return 0;
        w->high = (uint8_t)atoi(h + (sizeof("\"high\":\"") - 1U));
        w->low = (uint8_t)atoi(l + (sizeof("\"low\":\"") - 1U));
        return 1;
    }
    return 0;
}

static void uart_puts(const char *s)
{
    while (*s) {
        while (USART_GetFlagStatus(ESP_USART, USART_FLAG_TXE) == RESET) {}
        USART_SendData(ESP_USART, (uint8_t)*s++);
    }
}

void EspAt_Init(void)
{
    esp_rx_queue = xQueueCreate(ESP_RX_QUEUE_LENGTH, sizeof(uint8_t));
    configASSERT(esp_rx_queue != NULL);
    esp_uart_config();
}

uint8_t EspAt_ConfigureSntp(void)
{
    char response[96];

    uart_begin_command("AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\",\"ntp2.aliyun.com\"\r\n");
    return uart_wait_response(response, sizeof(response), 3000U);
}

uint8_t EspAt_IsWifiConnected(char *ssid, uint8_t ssid_size)
{
    char response[128];

    if (ssid != NULL && ssid_size > 0U)
        ssid[0] = 0;

    uart_begin_command("AT+CWSTATE?\r\n");
    if (uart_wait_response(response, sizeof(response), 1200U))
    {
        const char *state = strstr(response, "+CWSTATE:");
        if (state != NULL && state[9] == '2' &&
            (state[10] == ',' || state[10] == '\r' || state[10] == '\n'))
        {
            const char *name = (state[10] == ',' && state[11] == '"') ? state + 12 : NULL;
            const char *end = name != NULL ? strchr(name, '"') : NULL;
            if (end != NULL && ssid != NULL && ssid_size > 0U)
            {
                uint8_t length = (uint8_t)(end - name);
                if (length >= ssid_size)
                    length = (uint8_t)(ssid_size - 1U);
                memcpy(ssid, name, length);
                ssid[length] = 0;
            }
            return 1U;
        }
    }
    return 0U;
}

static uint8_t month_number(const char *s)
{
    static const char *const names[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    uint8_t i;
    for (i = 0; i < 12; i++) if (memcmp(s, names[i], 3) == 0) return (uint8_t)(i + 1U);
    return 0;
}

static uint8_t weekday_number(const char *s)
{
    /* STM32 RTC uses Monday=1 ... Sunday=7. */
    static const char *const names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    uint8_t i;
    for (i = 0; i < 7; i++) if (memcmp(s, names[i], 3) == 0) return (uint8_t)(i + 1U);
    return 0;
}

uint8_t EspAt_SyncRtcFromSntp(weather_rtc_time_t *time)
{
    char weekday[8], month[4];
    unsigned day, hour, minute, second, year;
    char *p;
    uint8_t wd;
    uint8_t mo;

    if (!uart_wait_response(sntp_last_response, sizeof(sntp_last_response), 5000U))
        return 0U;

    p = strstr(sntp_last_response, "+CIPSNTPTIME:");
    if (p && sscanf(p, "+CIPSNTPTIME:%3s %3s %u %u:%u:%u %u",
                    weekday, month, &day, &hour, &minute, &second, &year) == 7)
    {
        wd = weekday_number(weekday);
        mo = month_number(month);
        if (wd && mo && year >= 2024U && year <= 2099U &&
            day >= 1U && day <= 31U && hour < 24U && minute < 60U && second < 60U)
        {
            time->weekday = wd; time->month = mo; time->day = (uint8_t)day;
            time->year = (uint8_t)(year % 100U); time->hour = (uint8_t)hour;
            time->minute = (uint8_t)minute; time->second = (uint8_t)second;
            return 1U;
        }
    }
    return 0;
}

const char *EspAt_LastTimeResponse(void)
{
    return sntp_last_response;
}

/* Request the current module time, then parse its direct response. */
uint8_t EspAt_RequestTime(weather_rtc_time_t *time)
{
    uart_begin_command("AT+CIPSNTPTIME?\r\n");
    return EspAt_SyncRtcFromSntp(time);
}

void USART3_IRQHandler(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint8_t byte;

    if (USART_GetITStatus(ESP_USART, USART_IT_RXNE) != RESET)
    {
        byte = (uint8_t)USART_ReceiveData(ESP_USART);
        if (esp_rx_queue != NULL)
            (void)xQueueSendFromISR(esp_rx_queue, &byte, &higher_priority_task_woken);
        USART_ClearITPendingBit(ESP_USART, USART_IT_RXNE);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}
