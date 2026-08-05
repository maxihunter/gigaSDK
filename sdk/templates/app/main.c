#include "main.h"

#include "audio/audio.h"
#include "fatfs.h"
#include "ff.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"
#include "keyboard.h"
#include "rtc/rtc_clock.h"
#include "video/video.h"

#include <stdio.h>

static FATFS app_filesystem;
static uint8_t card_mounted;

/*
 * Each subsystem has a separate initializer on purpose. If it is not needed,
 * remove its call from App_Init(). The build uses -ffunction-sections and
 * --gc-sections, so the linker will discard code that is no longer referenced.
 * You may also remove its source files from Makefile to make compilation faster.
 */
static void App_InitDisplay(void)
{
    ILI9341_Init();
}

static void App_InitStorage(void)
{
    card_mounted = (f_mount(&app_filesystem, SDPath, 1U) == FR_OK);
}

static void App_InitKeyboard(void)
{
    /* GPIO inputs are configured by App_PlatformInit(). No driver state needed. */
    (void)getKeyState();
}

static void App_InitAudio(void)
{
    if (Audio_Init(&hi2s3, NULL) != HAL_OK) Error_Handler();
    /* Start silence only when effects must work before background music starts. */
    if (Audio_MixerStartSilence() != HAL_OK) Error_Handler();
}

static void App_InitVideo(void)
{
    if (Video_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void App_InitClock(void)
{
    /* RTC is battery-backed. Existing date/time is preserved across resets. */
    if (RTC_Clock_Init() != HAL_OK) Error_Handler();
}

static void App_Init(void)
{
    App_PlatformInit();
    App_InitDisplay();  /* Remove this call if the application has no graphics. */
    App_InitStorage();  /* Remove this call if the SD card is not used. */
    App_InitKeyboard(); /* Remove this call if buttons are not used. */
    App_InitAudio();    /* Remove this call if audio is not used. */
    App_InitVideo();    /* Remove this call if .vid playback is not used. */
    App_InitClock();    /* Remove this call if date/time is not used. */
}

static void App_DrawScreen(void)
{
    char time_text[32] = "RTC unavailable";
    RTC_ClockDateTime now;

    if (RTC_Clock_Get(&now) == HAL_OK) {
        snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u  %02u.%02u.%04u",
                 now.hours, now.minutes, now.seconds,
                 now.day, now.month, now.year);
    }

    ILI9341_Fill_Screen(0x0864U);
    ILI9341_Draw_Text("GIGA APP TEMPLATE", 40U, 52U, WHITE, 2U, 0x0864U);
    ILI9341_Draw_Text(time_text, 64U, 102U, 0xFFE0U, 1U, 0x0864U);
    ILI9341_Draw_Text(card_mounted ? "SD CARD: READY" : "SD CARD: NOT READY",
                      88U, 126U, card_mounted ? GREEN : RED, 1U, 0x0864U);
    ILI9341_Draw_Text("A: click   B: reboot", 76U, 178U, LIGHTGREY, 1U, 0x0864U);
}

int main(void)
{
    App_Init();
    App_DrawScreen();

    uint32_t previous = getKeyState();
    uint32_t next_clock_redraw = HAL_GetTick() + 1000U;

    for (;;) {
        uint32_t keys = getKeyState();
        uint32_t pressed = keys & ~previous;
        previous = keys;

        if (pressed & KBRD_BTN_1) (void)Audio_PlayUiClick();
        if (pressed & KBRD_BTN_2) NVIC_SystemReset();

        /* Required while the audio mixer is active; call it frequently. */
        if (Audio_MixerProcess() != HAL_OK) (void)Audio_MixerStop();

        if ((int32_t)(HAL_GetTick() - next_clock_redraw) >= 0) {
            App_DrawScreen();
            next_clock_redraw += 1000U;
        }
        HAL_Delay(5U);
    }
}
