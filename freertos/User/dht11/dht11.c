#include "./dht11/dht11.h"

#define DHT11_TIMEOUT_US  120U

static void dht11_delay_us(uint32_t microseconds)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = microseconds * (SystemCoreClock / 1000000U);

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
    }
}

static void dht11_set_output(void)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = DHT11_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void dht11_set_input(void)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = DHT11_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static uint8_t dht11_wait_level(BitAction level)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_cycles = DHT11_TIMEOUT_US * (SystemCoreClock / 1000000U);

    while (GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) != level)
    {
        if ((uint32_t)(DWT->CYCCNT - start) >= timeout_cycles)
        {
            return 0;
        }
    }
    return 1;
}

void DHT11_Init(void)
{
    RCC_AHB1PeriphClockCmd(DHT11_GPIO_CLOCK, ENABLE);
    dht11_set_input();

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint8_t DHT11_Read(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = { 0 };
    uint8_t byte_index;
    uint8_t bit_index;

    if ((temperature == 0) || (humidity == 0))
    {
        return 0;
    }

    dht11_set_output();
    GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    dht11_delay_us(20000U);
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    dht11_delay_us(30U);
    dht11_set_input();

    if ((!dht11_wait_level(Bit_RESET)) ||
        (!dht11_wait_level(Bit_SET)) ||
        (!dht11_wait_level(Bit_RESET)))
    {
        return 0;
    }

    for (byte_index = 0; byte_index < 5U; byte_index++)
    {
        for (bit_index = 0; bit_index < 8U; bit_index++)
        {
            if (!dht11_wait_level(Bit_SET))
            {
                return 0;
            }

            dht11_delay_us(40U);
            data[byte_index] <<= 1;
            if (GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_SET)
            {
                data[byte_index] |= 1U;
            }

            if (!dht11_wait_level(Bit_RESET))
            {
                return 0;
            }
        }
    }

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
    {
        return 0;
    }

    *humidity = data[0];
    *temperature = data[2];
    return 1;
}
