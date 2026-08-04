#include "main.h"
#include "fatfs.h"
#include "audio/audio.h"
#include "keyboard.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"
#include "ff.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PLAYER_DIR "0:/apps/player"
#define TRACK_LIMIT 24U
#define NAME_SIZE 13U
#define BG 0x1086U
#define PANEL 0x212CU
#define ACCENT 0x4E7FU
#define ACCENT2 0xFD8EU

typedef enum { TRACK_PCM, TRACK_GIM } TrackType;
typedef struct { char name[NAME_SIZE]; TrackType type; FSIZE_t size; } Track;

static FATFS filesystem;
static Track tracks[TRACK_LIMIT];
static size_t track_count;
static size_t selected;
static bool playing;
static uint16_t volume = 24575U;

static bool has_extension(const char *name, const char *extension)
{
    size_t n = strlen(name), e = strlen(extension);
    if (n < e) return false;
    name += n - e;
    while (*extension) {
        char a = *name++, b = *extension++;
        if ((a >= 'a') && (a <= 'z')) a -= 32;
        if ((b >= 'a') && (b <= 'z')) b -= 32;
        if (a != b) return false;
    }
    return true;
}

static void scan_tracks(void)
{
    DIR directory;
    FILINFO entry;
    track_count = 0U;
    if (f_opendir(&directory, PLAYER_DIR) != FR_OK) return;
    while ((track_count < TRACK_LIMIT) &&
           (f_readdir(&directory, &entry) == FR_OK) && entry.fname[0]) {
        if ((entry.fattrib & AM_DIR) != 0U) continue;
        TrackType type;
        if (has_extension(entry.fname, ".PCM")) type = TRACK_PCM;
        else if (has_extension(entry.fname, ".GIM")) type = TRACK_GIM;
        else continue;
        strncpy(tracks[track_count].name, entry.fname, NAME_SIZE - 1U);
        tracks[track_count].name[NAME_SIZE - 1U] = '\0';
        tracks[track_count].type = type;
        tracks[track_count].size = entry.fsize;
        track_count++;
    }
    (void)f_closedir(&directory);
}

static void draw_background(void)
{
    ILI9341_Fill_Screen(BG);
    ILI9341_Draw_Rectangle(0U, 0U, 320U, 38U, 0x18EAU);
    ILI9341_Draw_Text("GIGA PLAYER", 16U, 12U, WHITE, 2U, 0x18EAU);
    ILI9341_Draw_Text("PCM  /  GIMA", 214U, 16U, 0x9D7FU, 1U, 0x18EAU);
    ILI9341_Draw_Rectangle(12U, 52U, 296U, 92U, PANEL);
    ILI9341_Draw_Rectangle(12U, 153U, 296U, 48U, PANEL);
    ILI9341_Draw_Text("UP/DOWN track   A play/stop", 16U, 215U,
                      LIGHTGREY, 1U, BG);
    ILI9341_Draw_Text("LEFT/RIGHT volume   B reboot", 16U, 227U,
                      LIGHTGREY, 1U, BG);
}

static void draw_player(void)
{
    ILI9341_Draw_Rectangle(13U, 53U, 294U, 90U, PANEL);
    if (track_count == 0U) {
        ILI9341_Draw_Text("No .PCM or .GIM files", 34U, 83U,
                          WHITE, 2U, PANEL);
        ILI9341_Draw_Text("Copy music beside app.bin", 54U, 112U,
                          LIGHTGREY, 1U, PANEL);
    } else {
        const Track *track = &tracks[selected];
        char size_text[28];
        ILI9341_Draw_Text(playing ? "PLAYING" : "READY", 24U, 62U,
                          playing ? ACCENT2 : ACCENT, 1U, PANEL);
        ILI9341_Draw_Text(track->name, 24U, 82U, WHITE, 2U, PANEL);
        snprintf(size_text, sizeof(size_text), "%s  %lu KB",
                 track->type == TRACK_GIM ? "GIMA ADPCM" : "PCM 16-bit",
                 (unsigned long)(track->size / 1024U));
        ILI9341_Draw_Text(size_text, 24U, 116U, LIGHTGREY, 1U, PANEL);
        for (uint16_t i = 0U; i < 6U; ++i) {
            uint16_t height = (uint16_t)(10U + ((selected * 13U + i * 9U) % 31U));
            ILI9341_Draw_Rectangle((uint16_t)(250U + i * 7U),
                                   (uint16_t)(132U - height), 4U, height,
                                   playing ? ACCENT2 : ACCENT);
        }
    }
    ILI9341_Draw_Rectangle(13U, 154U, 294U, 46U, PANEL);
    ILI9341_Draw_Text("VOLUME", 24U, 163U, LIGHTGREY, 1U, PANEL);
    ILI9341_Draw_Rectangle(24U, 181U, 264U, 8U, 0x39CEU);
    uint16_t width = (uint16_t)(((uint32_t)volume * 264U) / 32767U);
    if (width) ILI9341_Draw_Rectangle(24U, 181U, width, 8U, ACCENT);
}

static void stop_playback(void)
{
    if (Audio_MixerIsRunning()) (void)Audio_MixerStop();
    playing = false;
}

static void start_playback(void)
{
    if (track_count == 0U) return;
    char path[48];
    stop_playback();
    snprintf(path, sizeof(path), PLAYER_DIR "/%s", tracks[selected].name);
    HAL_StatusTypeDef status = (tracks[selected].type == TRACK_GIM) ?
        Audio_MixerStartImaAdpcmMusic(path, 0U) :
        Audio_MixerStartPcmMusic(path, 0U);
    if (status == HAL_OK) {
        Audio_MixerSetMusicVolume(volume);
        playing = true;
    }
}

int main(void)
{
    Player_PlatformInit();
    ILI9341_Init();
    if (f_mount(&filesystem, SDPath, 1U) != FR_OK) Error_Handler();
    scan_tracks();
    draw_background(); draw_player();

    uint32_t previous = getKeyState();
    for (;;) {
        if (Audio_MixerIsRunning()) {
            if (Audio_MixerProcess() != HAL_OK) stop_playback();
        } else if (playing) {
            playing = false;
            draw_player();
        }
        uint32_t state = getKeyState();
        uint32_t pressed = state & ~previous;
        previous = state;
        if (pressed & KBRD_BTN_1) {
            if (playing) stop_playback(); else start_playback();
            draw_player();
        } else if ((pressed & KBRD_BTN_2) || (pressed & KBRD_BTN_MENU)) {
            stop_playback(); NVIC_SystemReset();
        } else if ((pressed & KBRD_BTN_UP) && track_count) {
            stop_playback(); selected = selected ? selected - 1U : track_count - 1U;
            draw_player();
        } else if ((pressed & KBRD_BTN_DOWN) && track_count) {
            stop_playback(); selected = (selected + 1U) % track_count;
            draw_player();
        } else if (pressed & KBRD_BTN_LEFT) {
            volume = (volume > 4096U) ? (uint16_t)(volume - 4096U) : 0U;
            Audio_MixerSetMusicVolume(volume); draw_player();
        } else if (pressed & KBRD_BTN_RIGHT) {
            volume = (volume < 28671U) ? (uint16_t)(volume + 4096U) : 32767U;
            Audio_MixerSetMusicVolume(volume); draw_player();
        }
        HAL_Delay(5U);
    }
}
