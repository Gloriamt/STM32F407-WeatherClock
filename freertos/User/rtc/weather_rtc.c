#include "./rtc/weather_rtc.h"

#define WEATHER_RTC_CONFIG_MARKER      0xA55AU
#define WEATHER_RTC_TIME_VALID_MARKER  0x5AA5U
#define WEATHER_RTC_LSE_TIMEOUT        0x1FFFFU
#define WEATHER_RTC_LSI_TIMEOUT        0xFFFFU

static uint8_t weather_rtc_start_lse(void)
{
    uint32_t timeout = WEATHER_RTC_LSE_TIMEOUT;

    RCC_LSEConfig(RCC_LSE_ON);
    while ((RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) && (timeout != 0U))
    {
        timeout--;
    }

    return (timeout != 0U) ? 1U : 0U;
}

static uint8_t weather_rtc_start_lsi(void)
{
    uint32_t timeout = WEATHER_RTC_LSI_TIMEOUT;

    RCC_LSICmd(ENABLE);
    while ((RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) && (timeout != 0U))
    {
        timeout--;
    }

    return (timeout != 0U) ? 1U : 0U;
}

static void weather_rtc_set_initial_calendar(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    time.RTC_H12 = RTC_H12_AM;
    time.RTC_Hours = 0U;
    time.RTC_Minutes = 0U;
    time.RTC_Seconds = 0U;
    RTC_SetTime(RTC_Format_BIN, &time);

    date.RTC_Year = 0U;
    date.RTC_Month = 1U;
    date.RTC_Date = 1U;
    date.RTC_WeekDay = RTC_Weekday_Saturday;
    RTC_SetDate(RTC_Format_BIN, &date);
}

static uint8_t weather_rtc_value_is_valid(const weather_rtc_time_t *value)
{
    static const uint8_t days_in_month[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };
    uint8_t maximum_day;

    if (value == 0 || value->year < 24U || value->year > 99U ||
        value->month < 1U || value->month > 12U || value->day < 1U ||
        value->weekday < RTC_Weekday_Monday ||
        value->weekday > RTC_Weekday_Sunday || value->hour > 23U ||
        value->minute > 59U || value->second > 59U)
    {
        return 0U;
    }

    maximum_day = days_in_month[value->month - 1U];
    if (value->month == 2U && (value->year % 4U) == 0U)
        maximum_day = 29U;

    return (value->day <= maximum_day) ? 1U : 0U;
}

static uint8_t weather_rtc_calendar_is_valid(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    RTC_GetTime(RTC_Format_BIN, &time);
    RTC_GetDate(RTC_Format_BIN, &date);

    if ((date.RTC_Month < 1U) || (date.RTC_Month > 12U) ||
        (date.RTC_Date < 1U) ||
        (date.RTC_Date > 31U) || (time.RTC_Hours > 23U) ||
        (time.RTC_Minutes > 59U) || (time.RTC_Seconds > 59U) ||
        (date.RTC_WeekDay < RTC_Weekday_Monday) ||
        (date.RTC_WeekDay > RTC_Weekday_Sunday))
    {
        return 0U;
    }

    return 1U;
}

uint8_t WeatherRtc_Init(void)
{
    RTC_InitTypeDef init;
    uint8_t use_lse;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if (RTC_ReadBackupRegister(RTC_BKP_DR0) == WEATHER_RTC_CONFIG_MARKER)
    {
        /* A system reset clears RCC enable bits but not the backup domain. */
        RCC_LSEConfig(RCC_LSE_ON);
        RCC_LSICmd(ENABLE);
        RCC_RTCCLKCmd(ENABLE);
        RTC_WaitForSynchro();

        if (!weather_rtc_calendar_is_valid())
        {
            weather_rtc_set_initial_calendar();
            RTC_WriteBackupRegister(RTC_BKP_DR1, 0U);
        }
        return 1U;
    }

    RCC_BackupResetCmd(ENABLE);
    RCC_BackupResetCmd(DISABLE);
    PWR_BackupAccessCmd(ENABLE);

    use_lse = weather_rtc_start_lse();
    if (use_lse)
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    }
    else
    {
        if (!weather_rtc_start_lsi())
        {
            return 0U;
        }
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    }

    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();

    init.RTC_HourFormat = RTC_HourFormat_24;
    init.RTC_AsynchPrediv = 127U;
    /* LSE is exactly 32768 Hz; LSI is nominally 32000 Hz. */
    init.RTC_SynchPrediv = use_lse ? 255U : 249U;
    RTC_Init(&init);
    weather_rtc_set_initial_calendar();
    RTC_WriteBackupRegister(RTC_BKP_DR0, WEATHER_RTC_CONFIG_MARKER);
    RTC_WriteBackupRegister(RTC_BKP_DR1, 0U);

    return 1U;
}

uint8_t WeatherRtc_IsTimeValid(void)
{
    return (RTC_ReadBackupRegister(RTC_BKP_DR1) ==
            WEATHER_RTC_TIME_VALID_MARKER) && weather_rtc_calendar_is_valid();
}

void WeatherRtc_Get(weather_rtc_time_t *time)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    if (time == 0)
    {
        return;
    }

    RTC_GetTime(RTC_Format_BIN, &rtc_time);
    RTC_GetDate(RTC_Format_BIN, &rtc_date);

    time->year = rtc_date.RTC_Year;
    time->month = rtc_date.RTC_Month;
    time->day = rtc_date.RTC_Date;
    time->weekday = rtc_date.RTC_WeekDay;
    time->hour = rtc_time.RTC_Hours;
    time->minute = rtc_time.RTC_Minutes;
    time->second = rtc_time.RTC_Seconds;
}

uint8_t WeatherRtc_Set(const weather_rtc_time_t *value)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    if (!weather_rtc_value_is_valid(value))
        return 0U;

    rtc_time.RTC_H12 = RTC_H12_AM;
    rtc_time.RTC_Hours = value->hour;
    rtc_time.RTC_Minutes = value->minute;
    rtc_time.RTC_Seconds = value->second;
    rtc_date.RTC_Year = value->year;
    rtc_date.RTC_Month = value->month;
    rtc_date.RTC_Date = value->day;
    rtc_date.RTC_WeekDay = value->weekday;
    RTC_WriteBackupRegister(RTC_BKP_DR1, 0U);
    if (RTC_SetTime(RTC_Format_BIN, &rtc_time) != SUCCESS ||
        RTC_SetDate(RTC_Format_BIN, &rtc_date) != SUCCESS)
    {
        return 0U;
    }

    RTC_WriteBackupRegister(RTC_BKP_DR1, WEATHER_RTC_TIME_VALID_MARKER);
    return 1U;
}
