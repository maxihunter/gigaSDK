#include "main.h"
#include "keyboard.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"
#include "audio/audio.h"
#include "fatfs.h"
#include "fonts/FreeSerif9pt7b.h"
#include "fonts/FreeSerif12pt7b.h"

#include <stdbool.h>
#include <stdio.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define COURT 0x0864U
#define PANEL 0x190AU
#define CYAN_NEON 0x4E7FU
#define PINK_NEON 0xF9B8U
#define GOLD 0xFDC6U
#define PADDLE_W 7
#define PADDLE_H 42
#define BALL_SIZE 7
#define PLAYER_X 15
#define AI_X 298
#define WIN_SCORE 7
#define MUSIC_FILE_IMA "0:/apps/pong/pong_bg.gim"


typedef struct {
    int16_t player_y, ai_y, ball_x, ball_y, ball_vx, ball_vy;
    uint8_t player_score, ai_score;
} Game;

static Game game;
static uint8_t difficulty = 1U;
static uint8_t menu_item;
static uint8_t music_enabled = 1U;
static uint8_t storage_ready;
static FATFS filesystem;

static void audio_service(void)
{
    if (Audio_MixerIsRunning() && (Audio_MixerProcess() != HAL_OK))
        (void)Audio_MixerStop();
}

static void audio_delay(uint32_t milliseconds)
{
    uint32_t deadline = HAL_GetTick() + milliseconds;
    while ((int32_t)(deadline - HAL_GetTick()) > 0) {
        audio_service(); HAL_Delay(1U);
    }
}

static void wait_keys_release(void)
{
    while (getKeyState()) { audio_service(); HAL_Delay(5U); }
}

static void start_music(void)
{
    if (music_enabled && storage_ready)
        (void)Audio_MixerStartImaAdpcmMusic(MUSIC_FILE_IMA, 1U);
    else if (!Audio_MixerIsRunning())
        (void)Audio_MixerStartSilence();
}

static void fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ILI9341_Draw_Filled_Rectangle_Coord(x, y, (uint16_t)(x + w - 1U),
                                        (uint16_t)(y + h - 1U), color);
}

static void draw_title_logo(void)
{
    ILI9341_Draw_Text("P", 94U, 32U, CYAN_NEON, 5U, COURT);
    ILI9341_Draw_Text("O", 130U, 32U, WHITE, 5U, COURT);
    ILI9341_Draw_Text("N", 166U, 32U, PINK_NEON, 5U, COURT);
    ILI9341_Draw_Text("G", 202U, 32U, GOLD, 5U, COURT);
    fill(73U, 87U, 174U, 2U, CYAN_NEON);
    ILI9341_Draw_Text("NEON TABLE", 121U, 96U, LIGHTGREY, 1U, COURT);
}

static void draw_menu_row(uint8_t row, const char *label, bool selected)
{
    uint16_t y = (uint16_t)(113U + row * 27U);
    uint16_t color = selected ? 0x31B0U : PANEL;
    fill(70U, y, 180U, 22U, color);
    if (selected) {
        fill(70U, y, 4U, 22U, row == 0U ? CYAN_NEON : PINK_NEON);
        ILI9341_Draw_Text(">", 82U, (uint16_t)(y + 7U), WHITE, 1U, color);
    }
    ILI9341_Draw_Text(label, 105U, (uint16_t)(y + 7U), WHITE, 1U, color);
}

static void draw_menu_item(uint8_t row)
{
    if (row == 0U) {
        draw_menu_row(row, "START MATCH", menu_item == row);
    } else if (row == 1U) {
        char level[24];
        snprintf(level, sizeof(level), "AI: %s", difficulty == 0U ? "EASY" :
                 (difficulty == 1U ? "NORMAL" : "HARD"));
        draw_menu_row(row, level, menu_item == row);
    } else if (row == 2U) {
        draw_menu_row(row, music_enabled ? "MUSIC: ON" : "MUSIC: OFF",
                      menu_item == row);
    } else {
        draw_menu_row(row, "RETURN TO BIOS", menu_item == row);
    }
}

static void draw_main_menu(void)
{
    ILI9341_Fill_Screen(COURT);
    for (uint16_t y = 0U; y < 28U; y += 4U) fill(0U, y, 320U, 4U, (uint16_t)(0x0864U + y * 2U));
    draw_title_logo();
    draw_menu_item(0U);
    draw_menu_item(1U);
    draw_menu_item(2U);
    draw_menu_item(3U);
    ILI9341_Draw_Text("UP/DOWN   A SELECT", 99U, 226U, LIGHTGREY, 1U, COURT);
}

static void draw_center_line(void)
{
    for (uint16_t y = 25U; y < 240U; y += 16U) fill(159U, y, 2U, 8U, 0x39CEU);
}

static void draw_scores(void)
{
    fill(105U, 0U, 110U, 25U, COURT);
    char score[12]; snprintf(score, sizeof(score), "%u   %u", game.player_score, game.ai_score);
    ILI9341_Draw_Text(score, 132U, 6U, WHITE, 2U, COURT);
}

static void draw_court(void)
{
    ILI9341_Fill_Screen(COURT);
    fill(0U, 22U, 320U, 2U, 0x31AEU); draw_center_line(); draw_scores();
    ILI9341_Draw_Text("YOU", 18U, 7U, CYAN_NEON, 1U, COURT);
    ILI9341_Draw_Text("CPU", 240U, 7U, PINK_NEON, 1U, COURT);
}

static void serve(int8_t direction)
{
    game.ball_x = (SCREEN_W - BALL_SIZE) / 2;
    game.ball_y = 70 + (int16_t)(HAL_GetTick() % 90U);
    game.ball_vx = (int16_t)(direction * (difficulty == 2U ? 4 : 3));
    game.ball_vy = (HAL_GetTick() & 1U) ? 2 : -2;
}

static void reset_match(void)
{
    game.player_y = game.ai_y = (SCREEN_H - PADDLE_H) / 2;
    game.player_score = game.ai_score = 0U; serve(1);
}

static void erase_objects(void)
{
    fill(PLAYER_X, (uint16_t)game.player_y, PADDLE_W, PADDLE_H, COURT);
    fill(AI_X, (uint16_t)game.ai_y, PADDLE_W, PADDLE_H, COURT);
    fill((uint16_t)game.ball_x, (uint16_t)game.ball_y, BALL_SIZE, BALL_SIZE, COURT);
}

static void draw_objects(void)
{
    fill(PLAYER_X, (uint16_t)game.player_y, PADDLE_W, PADDLE_H, CYAN_NEON);
    fill(AI_X, (uint16_t)game.ai_y, PADDLE_W, PADDLE_H, PINK_NEON);
    fill((uint16_t)game.ball_x, (uint16_t)game.ball_y, BALL_SIZE, BALL_SIZE, GOLD);
    fill((uint16_t)(game.ball_x + 2), (uint16_t)(game.ball_y + 2), 3U, 3U, WHITE);
}

static void update_game(uint32_t keys)
{
    if ((keys & KBRD_BTN_UP) && game.player_y > 27) game.player_y -= 4;
    if ((keys & KBRD_BTN_DOWN) && game.player_y < SCREEN_H - PADDLE_H - 2) game.player_y += 4;

    int16_t target = game.ball_y + BALL_SIZE / 2 - PADDLE_H / 2;
    int16_t ai_step = difficulty == 0U ? 1 : (difficulty == 1U ? 2 : 3);
    if ((game.ball_vx > 0) || difficulty == 2U) {
        if (game.ai_y < target) game.ai_y += ai_step;
        else if (game.ai_y > target) game.ai_y -= ai_step;
    }
    if (game.ai_y < 25) game.ai_y = 25;
    if (game.ai_y > SCREEN_H - PADDLE_H - 2) game.ai_y = SCREEN_H - PADDLE_H - 2;

    game.ball_x += game.ball_vx; game.ball_y += game.ball_vy;
    if (game.ball_y <= 25) {
        game.ball_y = 25; game.ball_vy = -game.ball_vy;
        (void)Audio_PlayUiBounce();
    }
    if (game.ball_y >= SCREEN_H - BALL_SIZE) {
        game.ball_y = SCREEN_H - BALL_SIZE; game.ball_vy = -game.ball_vy;
        (void)Audio_PlayUiBounce();
    }
    if ((game.ball_vx < 0) && (game.ball_x <= PLAYER_X + PADDLE_W) &&
        (game.ball_x + BALL_SIZE >= PLAYER_X) &&
        (game.ball_y + BALL_SIZE >= game.player_y) &&
        (game.ball_y <= game.player_y + PADDLE_H)) {
        game.ball_x = PLAYER_X + PADDLE_W; game.ball_vx = -game.ball_vx;
        game.ball_vy += (game.ball_y - game.player_y - PADDLE_H / 2) / 9;
        (void)Audio_PlayUiBounce();
    }
    if ((game.ball_vx > 0) && (game.ball_x + BALL_SIZE >= AI_X) &&
        (game.ball_y + BALL_SIZE >= game.ai_y) &&
        (game.ball_y <= game.ai_y + PADDLE_H)) {
        game.ball_x = AI_X - BALL_SIZE; game.ball_vx = -game.ball_vx;
        game.ball_vy += (game.ball_y - game.ai_y - PADDLE_H / 2) / 10;
        (void)Audio_PlayUiBounce();
    }
    if (game.ball_vy > 4) game.ball_vy = 4;
    if (game.ball_vy < -4) game.ball_vy = -4;
    if ((game.ball_vx < 0) && (game.ball_x <= 0)) {
        game.ai_score++; draw_scores(); serve(1);
    }
    if ((game.ball_vx > 0) && (game.ball_x + BALL_SIZE >= SCREEN_W)) {
        game.player_score++; draw_scores(); serve(-1);
    }
}

static bool pause_menu(void)
{
    fill(68U, 76U, 184U, 88U, 0x212CU);
    ILI9341_Draw_Hollow_Rectangle_Coord(68U, 76U, 251U, 163U, CYAN_NEON);
    ILI9341_Draw_Text("PAUSED", 105U, 91U, WHITE, 3U, 0x212CU);
    ILI9341_Draw_Text("A RESUME", 124U, 132U, CYAN_NEON, 1U, 0x212CU);
    ILI9341_Draw_Text("B QUIT", 130U, 148U, PINK_NEON, 1U, 0x212CU);
    wait_keys_release();
    for (;;) {
        audio_service();
        uint32_t key = getKeyState();
        if (key & KBRD_BTN_1) { wait_keys_release(); draw_court(); draw_objects(); return true; }
        if (key & KBRD_BTN_2) { wait_keys_release(); return false; }
        HAL_Delay(5U);
    }
}

static void game_over(void)
{
    bool won = game.player_score >= WIN_SCORE;
    fill(49U, 70U, 222U, 100U, 0x212CU);
    ILI9341_Draw_Hollow_Rectangle_Coord(49U, 70U, 270U, 169U, won ? CYAN_NEON : PINK_NEON);
    ILI9341_Draw_Text(won ? "YOU WIN!" : "CPU WINS", 78U, 89U,
                      won ? CYAN_NEON : PINK_NEON, 3U, 0x212CU);
    ILI9341_Draw_Text("A  MAIN MENU", 112U, 145U, WHITE, 1U, 0x212CU);
    wait_keys_release();
    while (!(getKeyState() & (KBRD_BTN_1 | KBRD_BTN_2))) {
        audio_service(); HAL_Delay(5U);
    }
    wait_keys_release();
}

static void play_match(void)
{
    reset_match(); draw_court(); draw_objects();
    uint32_t previous = 0U, next_frame = HAL_GetTick();
    while ((game.player_score < WIN_SCORE) && (game.ai_score < WIN_SCORE)) {
        audio_service();
        uint32_t now = HAL_GetTick();
        if ((int32_t)(now - next_frame) < 0) { HAL_Delay(1U); continue; }
        next_frame += 20U;
        uint32_t keys = getKeyState(), pressed = keys & ~previous; previous = keys;
        if (pressed & KBRD_BTN_2) { if (!pause_menu()) return; previous = getKeyState(); }
        erase_objects(); draw_center_line(); update_game(keys); draw_objects();
    }
    game_over();
}

int main(void)
{
    Pong_PlatformInit(); ILI9341_Init();
    storage_ready = (f_mount(&filesystem, SDPath, 1U) == FR_OK) ? 1U : 0U;
    start_music();
    ILI9341_Fill_Screen(BLACK);
    audio_delay(1000U);
    ILI9341_Draw_Text_Font("A Maxi_Hunter's game", 40, 100, WHITE, 1, BLACK, &FreeSerif12pt7b);
    audio_delay(4000U);
    ILI9341_Fill_Screen(BLACK);
    audio_delay(1000U);
    //Drugs Of Choice (cdk Mix) by cdk (c) copyright 2026 Licensed under a Creative Commons Attribution Noncommercial  (3.0) license
    ILI9341_Draw_Text_Font("Music:", 100, 40, WHITE, 1, BLACK, &FreeSerif12pt7b);
    ILI9341_Draw_Text_Font("Drugs Of Choice (cdk Mix)", 50, 80, WHITE, 1, BLACK, &FreeSerif9pt7b);
    ILI9341_Draw_Text_Font("by cdk (c) copyright 2026", 50, 100, WHITE, 1, BLACK, &FreeSerif9pt7b);
    ILI9341_Draw_Text_Font("Licensed under a Creative Commons", 30, 120, WHITE, 1, BLACK, &FreeSerif9pt7b);
    ILI9341_Draw_Text_Font("Attribution Noncommercial (3.0) license", 10, 140, WHITE, 1, BLACK, &FreeSerif9pt7b);
    audio_delay(5000U);
    
    draw_main_menu();
    uint32_t previous = getKeyState();
    for (;;) {
        audio_service();
        uint32_t keys = getKeyState(), pressed = keys & ~previous; previous = keys;
        /* As on the BIOS menu, SELECT has priority: this keyboard revision can
           briefly report DOWN together with A while the A contact settles. */
        if (pressed & KBRD_BTN_1) {
            if (menu_item == 0U) { (void)Audio_PlayUiClick(); wait_keys_release(); play_match(); draw_main_menu(); previous = 0U; }
            else if (menu_item == 1U) { difficulty = (difficulty + 1U) % 3U; (void)Audio_PlayUiClick(); draw_menu_item(1U); }
            else if (menu_item == 2U) {
                music_enabled ^= 1U;
                if (music_enabled) start_music(); else (void)Audio_MixerStop();
                (void)Audio_PlayUiClick(); draw_menu_item(2U);
            }
            else NVIC_SystemReset();
        } else if (pressed & KBRD_BTN_2) {
            NVIC_SystemReset();
        } else if (pressed & KBRD_BTN_UP) {
            uint8_t old_item = menu_item;
            menu_item = menu_item ? menu_item - 1U : 3U;
            (void)Audio_PlayUiClick();
            draw_menu_item(old_item); draw_menu_item(menu_item);
        }
        else if (pressed & KBRD_BTN_DOWN) {
            uint8_t old_item = menu_item;
            menu_item = (menu_item + 1U) % 4U;
            (void)Audio_PlayUiClick();
            draw_menu_item(old_item); draw_menu_item(menu_item);
        }
        else if ((pressed & (KBRD_BTN_LEFT | KBRD_BTN_RIGHT)) && menu_item == 1U) {
            difficulty = (pressed & KBRD_BTN_RIGHT) ? (difficulty + 1U) % 3U :
                         (difficulty ? difficulty - 1U : 2U);
            (void)Audio_PlayUiClick(); draw_menu_item(1U);
        } else if ((pressed & (KBRD_BTN_LEFT | KBRD_BTN_RIGHT)) && menu_item == 2U) {
            music_enabled ^= 1U;
            if (music_enabled) start_music(); else (void)Audio_MixerStop();
            (void)Audio_PlayUiClick(); draw_menu_item(2U);
        }
        HAL_Delay(5U);
    }
}
