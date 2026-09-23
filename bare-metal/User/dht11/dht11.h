#ifndef DHT11_H
#define DHT11_H

#include "stm32f4xx.h"

/* Batianhu V2 J5: DHT11 DATA is connected to PE3 with a 10 kOhm pull-up. */
#define DHT11_GPIO_PORT       GPIOE
#define DHT11_GPIO_CLOCK      RCC_AHB1Periph_GPIOE
#define DHT11_GPIO_PIN        GPIO_Pin_3

void DHT11_Init(void);
uint8_t DHT11_Read(uint8_t *temperature, uint8_t *humidity);

#endif
