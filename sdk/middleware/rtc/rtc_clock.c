#include "rtc/rtc_clock.h"

#define RTC_CLOCK_BACKUP_MAGIC 0x47535452U

static RTC_HandleTypeDef hrtc;

static uint8_t RTC_Clock_DaysInMonth(uint16_t year, uint8_t month)
{
  static const uint8_t days[] = {
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
  };
  uint8_t result;

  if ((month < 1U) || (month > 12U))
  {
    return 0U;
  }

  result = days[month - 1U];
  if ((month == 2U) &&
      (((year % 400U) == 0U) ||
       (((year % 4U) == 0U) && ((year % 100U) != 0U))))
  {
    result = 29U;
  }
  return result;
}

static uint8_t RTC_Clock_Weekday(uint16_t year, uint8_t month, uint8_t day)
{
  static const uint8_t month_offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

  if (month < 3U)
  {
    --year;
  }

  /* Convert Sunday=0..Saturday=6 to HAL Monday=1..Sunday=7. */
  uint8_t sunday_based = (uint8_t)((year + year / 4U - year / 100U +
                                    year / 400U + month_offsets[month - 1U] +
                                    day) % 7U);
  return (sunday_based == 0U) ? RTC_WEEKDAY_SUNDAY : sunday_based;
}

HAL_StatusTypeDef RTC_Clock_Set(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  if ((year < 2000U) || (year > 2099U) ||
      (month < 1U) || (month > 12U) ||
      (day < 1U) || (day > RTC_Clock_DaysInMonth(year, month)) ||
      (hours > 23U) || (minutes > 59U) || (seconds > 59U))
  {
    return HAL_ERROR;
  }

  time.Hours = hours;
  time.Minutes = minutes;
  time.Seconds = seconds;
  time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  time.StoreOperation = RTC_STOREOPERATION_RESET;

  date.Year = (uint8_t)(year - 2000U);
  date.Month = month;
  date.Date = day;
  date.WeekDay = RTC_Clock_Weekday(year, month, day);

  if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_CLOCK_BACKUP_MAGIC);
  return HAL_OK;
}

HAL_StatusTypeDef RTC_Clock_Get(RTC_ClockDateTime *date_time)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  if (date_time == NULL)
  {
    return HAL_ERROR;
  }

  if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    return HAL_ERROR;
  }
  /* Reading the date unlocks the RTC shadow registers after reading time. */
  if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    return HAL_ERROR;
  }

  date_time->year = 2000U + date.Year;
  date_time->month = date.Month;
  date_time->day = date.Date;
  date_time->weekday = date.WeekDay;
  date_time->hours = time.Hours;
  date_time->minutes = time.Minutes;
  date_time->seconds = time.Seconds;
  return HAL_OK;
}

HAL_StatusTypeDef RTC_Clock_Init(void)
{
  RCC_OscInitTypeDef oscillator = {0};
  RCC_PeriphCLKInitTypeDef peripheral_clock = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  oscillator.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  oscillator.LSEState = RCC_LSE_ON;
  oscillator.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
  {
    return HAL_ERROR;
  }

  peripheral_clock.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  peripheral_clock.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&peripheral_clock) != HAL_OK)
  {
    return HAL_ERROR;
  }

  __HAL_RCC_RTC_ENABLE();

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != RTC_CLOCK_BACKUP_MAGIC)
  {
    return RTC_Clock_Set(2026U, 1U, 1U, 0U, 0U, 0U);
  }

  return HAL_OK;
}
