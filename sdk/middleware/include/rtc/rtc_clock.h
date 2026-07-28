#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include "stm32f4xx_hal.h"

typedef struct
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} RTC_ClockDateTime;

HAL_StatusTypeDef RTC_Clock_Init(void);
HAL_StatusTypeDef RTC_Clock_Get(RTC_ClockDateTime *date_time);
HAL_StatusTypeDef RTC_Clock_Set(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hours, uint8_t minutes, uint8_t seconds);

#endif
