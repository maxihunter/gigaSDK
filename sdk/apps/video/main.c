#include "main.h"
#include "fatfs.h"
#include "ff.h"
#include "keyboard.h"
#include "video/video.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define VIDEO_DIR "0:/apps/video"
#define VIDEO_LIMIT 20U
#define NAME_SIZE 13U
#define BG 0x0864U
#define PANEL 0x190AU
#define ACCENT 0xE3D0U
#define BLUE_ACCENT 0x34BFU

typedef struct { char name[NAME_SIZE]; Video_Info info; FSIZE_t size; } VideoEntry;
static FATFS filesystem;
static VideoEntry videos[VIDEO_LIMIT];
static size_t video_count;
static size_t selected;
static volatile uint8_t video_abort_requested;

static bool is_vid(const char *name)
{
    size_t length = strlen(name);
    if (length < 4U) return false;
    const char *ext = name + length - 4U;
    return (ext[0] == '.') && ((ext[1] | 32) == 'v') &&
           ((ext[2] | 32) == 'i') && ((ext[3] | 32) == 'd');
}

static void scan_videos(void)
{
    DIR directory; FILINFO file;
    video_count = 0U;
    if (f_opendir(&directory, VIDEO_DIR) != FR_OK) return;
    while ((video_count < VIDEO_LIMIT) &&
           (f_readdir(&directory, &file) == FR_OK) && file.fname[0]) {
        if ((file.fattrib & AM_DIR) || !is_vid(file.fname)) continue;
        char path[48]; Video_Info info;
        snprintf(path, sizeof(path), VIDEO_DIR "/%s", file.fname);
        if (Video_GetFileInfo(path, &info) != HAL_OK) continue;
        strncpy(videos[video_count].name, file.fname, NAME_SIZE - 1U);
        videos[video_count].name[NAME_SIZE - 1U] = '\0';
        videos[video_count].info = info; videos[video_count].size = file.fsize;
        video_count++;
    }
    (void)f_closedir(&directory);
}

static void draw_film_icon(void)
{
    ILI9341_Draw_Rectangle(224U, 57U, 68U, 72U, 0x294EU);
    ILI9341_Draw_Rectangle(231U, 64U, 54U, 58U, 0x0864U);
    for (uint16_t y = 62U; y < 124U; y += 13U) {
        ILI9341_Draw_Rectangle(226U, y, 4U, 7U, ACCENT);
        ILI9341_Draw_Rectangle(286U, y, 4U, 7U, ACCENT);
    }
    for (uint16_t row = 0U; row < 24U; ++row)
        ILI9341_Draw_Horizontal_Line((uint16_t)(247U - row / 2U),
                                    (uint16_t)(80U + row),
                                    (uint16_t)(row + 1U), BLUE_ACCENT);
}

static void draw_screen(void)
{
    ILI9341_Fill_Screen(BG);
    ILI9341_Draw_Rectangle(0U, 0U, 320U, 40U, 0x10C8U);
    ILI9341_Draw_Text("GIGA VIDEO", 16U, 12U, WHITE, 2U, 0x10C8U);
    ILI9341_Draw_Text("GVID", 220U, 16U, ACCENT, 1U, 0x10C8U);
    ILI9341_Draw_Rectangle(12U, 52U, 296U, 102U, PANEL);
    draw_film_icon();
    if (video_count == 0U) {
        ILI9341_Draw_Text("No .VID files", 26U, 76U, WHITE, 2U, PANEL);
        ILI9341_Draw_Text("Copy video beside", 26U, 108U, LIGHTGREY, 1U, PANEL);
        ILI9341_Draw_Text("app.bin", 26U, 122U, LIGHTGREY, 1U, PANEL);
    } else {
        const VideoEntry *video = &videos[selected];
        char details[32], duration[32];
        uint32_t seconds = (uint32_t)(((uint64_t)video->info.frame_count * 1000U) /
                                      video->info.fps_milli);
        snprintf(details, sizeof(details), "%ux%u  %lu.%03lu FPS",
                 video->info.width, video->info.height,
                 (unsigned long)(video->info.fps_milli / 1000U),
                 (unsigned long)(video->info.fps_milli % 1000U));
        snprintf(duration, sizeof(duration), "%lu:%02lu  %lu frames",
                 (unsigned long)(seconds / 60U), (unsigned long)(seconds % 60U),
                 (unsigned long)video->info.frame_count);
        ILI9341_Draw_Text("READY TO PLAY", 26U, 64U, ACCENT, 1U, PANEL);
        ILI9341_Draw_Text(video->name, 26U, 84U, WHITE, 2U, PANEL);
        ILI9341_Draw_Text(details, 26U, 116U, LIGHTGREY, 1U, PANEL);
        ILI9341_Draw_Text(duration, 26U, 132U, LIGHTGREY, 1U, PANEL);
    }
    ILI9341_Draw_Rectangle(12U, 166U, 296U, 34U, 0x212CU);
    ILI9341_Draw_Text("UP/DOWN select     A play", 22U, 176U, WHITE, 1U, 0x212CU);
    ILI9341_Draw_Text("During video: any key stops", 22U, 188U,
                      LIGHTGREY, 1U, 0x212CU);
    ILI9341_Draw_Text("B  return to BIOS", 92U, 220U, LIGHTGREY, 1U, BG);
}

static void service_video(void)
{
    if (getKeyState()) video_abort_requested = 1U;
}
static uint8_t abort_video(void) { return video_abort_requested; }
static void wait_release(void) { while (getKeyState()) HAL_Delay(5U); }

static void play_selected(void)
{
    if (video_count == 0U) return;
    char path[48];
    snprintf(path, sizeof(path), VIDEO_DIR "/%s", videos[selected].name);
    wait_release();
    video_abort_requested = 0U;
    (void)Video_PlayFile(path, service_video, abort_video);
    wait_release();
    draw_screen();
}

int main(void)
{
    VideoApp_PlatformInit(); ILI9341_Init();
    if ((Video_Init(&hspi1) != HAL_OK) ||
        (f_mount(&filesystem, SDPath, 1U) != FR_OK)) Error_Handler();
    scan_videos(); draw_screen();
    uint32_t previous = getKeyState();
    for (;;) {
        uint32_t state = getKeyState(), pressed = state & ~previous;
        previous = state;
        if ((pressed & KBRD_BTN_UP) && video_count) {
            selected = selected ? selected - 1U : video_count - 1U; draw_screen();
        } else if ((pressed & KBRD_BTN_DOWN) && video_count) {
            selected = (selected + 1U) % video_count; draw_screen();
        } else if (pressed & KBRD_BTN_1) {
            play_selected(); previous = getKeyState();
        } else if ((pressed & KBRD_BTN_2) || (pressed & KBRD_BTN_MENU)) {
            NVIC_SystemReset();
        }
        HAL_Delay(5U);
    }
}
