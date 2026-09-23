#include "esp_at.h"
#include "weather_secrets.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern volatile uint32_t g_system_ms;

#define ESP_USART              USART3
#define ESP_USART_CLK          RCC_APB1Periph_USART3
#define ESP_USART_TX_PORT      GPIOB
#define ESP_USART_RX_PORT      GPIOB
#define ESP_USART_TX_PIN       GPIO_Pin_10
#define ESP_USART_RX_PIN       GPIO_Pin_11
#define ESP_USART_TX_SOURCE    GPIO_PinSource10
#define ESP_USART_RX_SOURCE    GPIO_PinSource11
static char sntp_last_response[256];

static void esp_uart_config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
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
    USART_Cmd(ESP_USART, ENABLE);
}

static void uart_puts(const char *s);

uint8_t EspAt_RequestWeather(esp_weather_t *w)
{
    char cmd[260], buf[900]; uint16_t n = 0; uint32_t start = g_system_ms;
    sprintf(cmd, "AT+HTTPCLIENT=2,1,\"https://api.seniverse.com/v3/weather/now.json?key=%s&location=shanghai&language=en&unit=c\",,,2\r\n", WEATHER_API_KEY);
    while (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET) (void)USART_ReceiveData(ESP_USART);
    uart_puts(cmd);
    while ((g_system_ms - start) < 8000U) {
        if (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET) {
            if (n < sizeof(buf)-1U) buf[n++] = (char)USART_ReceiveData(ESP_USART); else (void)USART_ReceiveData(ESP_USART);
            buf[n] = 0;
            if (strstr(buf, "\r\nOK\r\n")) {
                char *p = strstr(buf, "\"temperature\":\"");
                char *c = strstr(buf, "\"code\":\"");
                if (!p || !c) return 0;
                w->temperature = (uint8_t)atoi(p + (sizeof("\"temperature\":\"") - 1U));
                w->code = (uint8_t)atoi(c + (sizeof("\"code\":\"") - 1U));
                return 1;
            }
        }
    }
    return 0;
}

uint8_t EspAt_RequestForecast(esp_weather_t *w)
{
    char cmd[280], buf[900]; uint16_t n=0; uint32_t start=g_system_ms;
    sprintf(cmd,"AT+HTTPCLIENT=2,1,\"https://api.seniverse.com/v3/weather/daily.json?key=%s&location=shanghai&language=en&unit=c&start=0&days=1\",,,2\r\n",WEATHER_API_KEY);
    uart_puts(cmd);
    while ((g_system_ms-start)<8000U) if(USART_GetFlagStatus(ESP_USART,USART_FLAG_RXNE)!=RESET){ if(n<sizeof(buf)-1U)buf[n++]=(char)USART_ReceiveData(ESP_USART);else(void)USART_ReceiveData(ESP_USART);buf[n]=0; if(strstr(buf,"\r\nOK\r\n")){char *h=strstr(buf,"\"high\":\"");char *l=strstr(buf,"\"low\":\"");if(!h||!l)return 0;w->high=(uint8_t)atoi(h+8);w->low=(uint8_t)atoi(l+7);return 1;}}
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
    esp_uart_config();
}

uint8_t EspAt_ConfigureSntp(void)
{
    char response[96];
    uint8_t pos = 0U;
    uint32_t start;

    while (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET)
        (void)USART_ReceiveData(ESP_USART);
    uart_puts("AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\",\"ntp2.aliyun.com\"\r\n");
    start = g_system_ms;
    response[0] = 0;

    while ((g_system_ms - start) < 3000U)
    {
        if (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET)
        {
            char ch = (char)USART_ReceiveData(ESP_USART);
            if (pos < sizeof(response) - 1U)
            {
                response[pos++] = ch;
                response[pos] = 0;
            }
            if (strstr(response, "\r\nOK\r\n") != NULL)
                return 1U;
            if (strstr(response, "\r\nERROR\r\n") != NULL)
                return 0U;
        }
    }
    return 0U;
}

uint8_t EspAt_IsWifiConnected(char *ssid, uint8_t ssid_size)
{
    char response[128];
    uint16_t pos = 0;
    uint32_t start;

    if (ssid != NULL && ssid_size > 0U)
        ssid[0] = 0;

    while (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET)
        (void)USART_ReceiveData(ESP_USART);
    uart_puts("AT+CWSTATE?\r\n");
    start = g_system_ms;
    response[0] = 0;

    while ((g_system_ms - start) < 1200U)
    {
        if (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET)
        {
            char ch = (char)USART_ReceiveData(ESP_USART);
            if (pos < sizeof(response) - 1U)
            {
                response[pos++] = ch;
                response[pos] = 0;
            }
            if (strstr(response, "\r\nOK\r\n") != NULL)
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
                return 0U;
            }
            if (strstr(response, "\r\nERROR\r\n") != NULL)
                return 0U;
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
    uint16_t pos = 0;
    uint32_t start = g_system_ms;
    sntp_last_response[0] = 0;
    while ((g_system_ms - start) < 5000U) {
        if (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET) {
            if (pos < sizeof(sntp_last_response) - 1U)
                sntp_last_response[pos++] = (char)USART_ReceiveData(ESP_USART);
            else
                (void)USART_ReceiveData(ESP_USART);
            sntp_last_response[pos] = 0;
            if (strstr(sntp_last_response, "\r\nOK\r\n") != NULL) {
                char weekday[8], month[4];
                unsigned day, hour, minute, second, year;
                char *p = strstr(sntp_last_response, "+CIPSNTPTIME:");
                if (p && sscanf(p, "+CIPSNTPTIME:%3s %3s %u %u:%u:%u %u",
                                weekday, month, &day, &hour, &minute, &second, &year) == 7) {
                    uint8_t wd = weekday_number(weekday);
                    uint8_t mo = month_number(month);
                    if (wd && mo && year >= 2024U && year <= 2099U &&
                        day >= 1U && day <= 31U && hour < 24U && minute < 60U && second < 60U) {
                        time->weekday = wd; time->month = mo; time->day = (uint8_t)day;
                        time->year = (uint8_t)(year % 100U); time->hour = (uint8_t)hour;
                        time->minute = (uint8_t)minute; time->second = (uint8_t)second;
                        return 1;
                    }
                }
                return 0;
            }
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
    while (USART_GetFlagStatus(ESP_USART, USART_FLAG_RXNE) != RESET) (void)USART_ReceiveData(ESP_USART);
    uart_puts("AT+CIPSNTPTIME?\r\n");
    return EspAt_SyncRtcFromSntp(time);
}
