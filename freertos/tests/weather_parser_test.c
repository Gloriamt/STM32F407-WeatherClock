#include <assert.h>
#include <stdio.h>
#include "../User/esp_at/weather_parser.h"

static void test_current_weather(void)
{
    esp_weather_t weather = {0};

    assert(WeatherParser_ParseCurrent(
        "+HTTPCLIENT:256,{\"temperature\":\"24\",\"code\":\"1\"}\r\nOK\r\n",
        &weather));
    assert(weather.temperature == 24);
    assert(weather.code == 1U);

    assert(WeatherParser_ParseCurrent(
        "{\"temperature\":\"-12\",\"code\":\"23\"}", &weather));
    assert(weather.temperature == -12);
    assert(weather.code == 23U);

    assert(WeatherParser_ParseCurrent(
        "{\"temperature\":\"-99\",\"code\":\"99\"}", &weather));
    assert(WeatherParser_ParseCurrent(
        "{\"temperature\":\"99\",\"code\":\"0\"}", &weather));
}

static void test_forecast(void)
{
    esp_weather_t weather = {0};

    assert(WeatherParser_ParseForecast(
        "{\"high\":\"3\",\"low\":\"-8\"}", &weather));
    assert(weather.high == 3);
    assert(weather.low == -8);

    assert(WeatherParser_ParseForecast(
        "{\"high\":\"99\",\"low\":\"-99\"}", &weather));
    assert(weather.high == 99);
    assert(weather.low == -99);
}

static void test_invalid_values_do_not_replace_weather(void)
{
    static const char *const invalid_current[] = {
        "{\"temperature\":\"\",\"code\":\"1\"}",
        "{\"temperature\":\"-\",\"code\":\"1\"}",
        "{\"temperature\":\"2.5\",\"code\":\"1\"}",
        "{\"temperature\":\"+2\",\"code\":\"1\"}",
        "{\"temperature\":\"100\",\"code\":\"1\"}",
        "{\"temperature\":\"-100\",\"code\":\"1\"}",
        "{\"temperature\":\"20\",\"code\":\"-1\"}",
        "{\"temperature\":\"20\",\"code\":\"100\"}",
        "{\"temperature\":\"20\"}",
        "{\"code\":\"1\"}",
        "{\"temperature\":20,\"code\":\"1\"}",
        "{\"temperature\":\"20\",\"code\":\"1",
        "+HTTPCLIENT:81,{\"status\":\"The API key is invalid.\","
        "\"status_code\":\"AP010003\"}\r\nOK\r\n"
    };
    esp_weather_t weather;
    unsigned int i;

    for (i = 0U; i < sizeof(invalid_current) / sizeof(invalid_current[0]); i++)
    {
        weather.temperature = -7;
        weather.code = 9U;
        assert(!WeatherParser_ParseCurrent(invalid_current[i], &weather));
        assert(weather.temperature == -7);
        assert(weather.code == 9U);
    }

    weather.high = 6;
    weather.low = -4;
    assert(!WeatherParser_ParseForecast(
        "{\"high\":\"6\",\"low\":\"bad\"}", &weather));
    assert(weather.high == 6);
    assert(weather.low == -4);
}

static void test_invalid_forecast_does_not_replace_weather(void)
{
    static const char *const invalid_forecast[] = {
        "{\"high\":\"\",\"low\":\"2\"}",
        "{\"high\":\"3\",\"low\":\"-\"}",
        "{\"high\":\"3.5\",\"low\":\"2\"}",
        "{\"high\":\"100\",\"low\":\"2\"}",
        "{\"high\":\"3\",\"low\":\"-100\"}",
        "{\"high\":\"3\"}",
        "{\"low\":\"2\"}",
        "{\"high\":3,\"low\":\"2\"}",
        "{\"high\":\"3\",\"low\":\"2"
    };
    esp_weather_t weather;
    unsigned int i;

    for (i = 0U; i < sizeof(invalid_forecast) /
                    sizeof(invalid_forecast[0]); i++)
    {
        weather.high = 8;
        weather.low = -3;
        assert(!WeatherParser_ParseForecast(invalid_forecast[i], &weather));
        assert(weather.high == 8);
        assert(weather.low == -3);
    }
}

static void test_null_arguments(void)
{
    esp_weather_t weather = {0};

    assert(!WeatherParser_ParseCurrent(NULL, &weather));
    assert(!WeatherParser_ParseCurrent("{}", NULL));
    assert(!WeatherParser_ParseForecast(NULL, &weather));
    assert(!WeatherParser_ParseForecast("{}", NULL));
}

int main(void)
{
    test_current_weather();
    test_forecast();
    test_invalid_values_do_not_replace_weather();
    test_invalid_forecast_does_not_replace_weather();
    test_null_arguments();
    puts("weather_parser_test: PASS");
    return 0;
}
