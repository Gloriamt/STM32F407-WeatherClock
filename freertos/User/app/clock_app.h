#ifndef WEATHER_CLOCK_APP_H
#define WEATHER_CLOCK_APP_H

#include <stdint.h>

void ClockApp_Run(void);
uint8_t ClockApp_RunStartupStage(void);
void ClockApp_TimeTask(void *argument);
void ClockApp_IndoorTask(void *argument);
void ClockApp_NetworkTask(void *argument);

#endif
