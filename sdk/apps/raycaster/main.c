#include "main.h"
#include "keyboard.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"

#include <math.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define VIEW_H 210
#define STATUS_H (SCREEN_H - VIEW_H)
#define ROWS_PER_BUFFER 4
#define RENDER_BUFFER_SIZE (SCREEN_W * 2 * ROWS_PER_BUFFER)
#define MAP_W 16
#define MAP_H 16
#define TEX_SIZE 16
#define MOVE_SPEED 0.0475f
#define ROT_COS 0.99820054f
#define ROT_SIN 0.05996401f

typedef struct {
    int16_t top;
    int16_t bottom;
    uint8_t tex_x;
    uint8_t wall;
    uint8_t dark;
} RayColumn;

/* Map and texture palettes are const, so they remain in application Flash. */
static const uint8_t world[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,2,2,2,0,0,3,3,3,3,0,0,0,0,1},
    {1,0,2,0,0,0,0,3,0,0,3,0,0,4,0,1},
    {1,0,2,0,0,0,0,3,0,0,3,0,0,4,0,1},
    {1,0,2,0,0,0,0,0,0,0,3,0,0,4,0,1},
    {1,0,2,2,0,0,0,0,0,0,3,0,0,4,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,4,0,1},
    {1,0,0,0,0,0,0,2,2,2,0,0,0,4,0,1},
    {1,0,3,3,0,0,0,2,0,0,0,0,0,4,0,1},
    {1,0,3,0,0,0,0,2,0,0,0,4,4,4,0,1},
    {1,0,3,0,0,0,0,2,0,0,0,0,0,0,0,1},
    {1,0,3,0,0,0,0,2,2,2,0,0,0,0,0,1},
    {1,0,3,3,3,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static const uint16_t wall_palette[4][3] = {
    {0xC186U, 0xE30AU, 0x9043U}, /* warm brick */
    {0x1B2FU, 0x34BFU, 0x09C9U}, /* cyan panels */
    {0x7A8FU, 0xB3D7U, 0x51CAU}, /* stone */
    {0x9A4CU, 0xF9B8U, 0x7028U}  /* magenta columns */
};

static RayColumn columns[SCREEN_W];
static uint8_t render_buffers[2][RENDER_BUFFER_SIZE];
static volatile uint8_t dma_busy;
static float pos_x = 3.5f, pos_y = 7.5f;
static float dir_x = 1.0f, dir_y = 0.0f;
static float plane_x = 0.0f, plane_y = 0.66f;

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
    if (spi == &hspi1) dma_busy = 0U;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi)
{
    if (spi == &hspi1) dma_busy = 0U;
}

static void draw_status_bar(void)
{
    ILI9341_Draw_Rectangle(0U, VIEW_H, SCREEN_W, STATUS_H, 0x1082U);
    ILI9341_Draw_Horizontal_Line(0U, VIEW_H, SCREEN_W, 0x34BFU);
    ILI9341_Draw_Text("FPS", 12U, 220U, 0xC618U, 1U, 0x1082U);
}

static void draw_fps(uint32_t fps)
{
    char text[4];
    if (fps > 999U) fps = 999U;
    if (fps >= 100U) {
        text[0] = (char)('0' + fps / 100U);
        text[1] = (char)('0' + (fps / 10U) % 10U);
        text[2] = (char)('0' + fps % 10U);
        text[3] = '\0';
    } else if (fps >= 10U) {
        text[0] = (char)('0' + fps / 10U);
        text[1] = (char)('0' + fps % 10U);
        text[2] = '\0';
    } else {
        text[0] = (char)('0' + fps);
        text[1] = '\0';
    }
    ILI9341_Draw_Rectangle(42U, 214U, 36U, 20U, 0x1082U);
    ILI9341_Draw_Text(text, 48U, 220U, WHITE, 1U, 0x1082U);
}

static uint16_t shade(uint16_t color, uint8_t dark)
{
    if (!dark) return color;
    return (uint16_t)((color >> 1U) & 0x7BEFU);
}

static uint16_t texture_pixel(uint8_t wall, uint8_t tx, uint8_t ty, uint8_t dark)
{
    const uint16_t *p = wall_palette[(wall - 1U) & 3U];
    uint8_t mortar = 0U;
    uint8_t accent = 0U;
    switch (wall) {
    case 1:
        mortar = (uint8_t)((ty == 0U) || ((tx + ((ty >> 2U) & 1U) * 4U) % 8U == 0U));
        break;
    case 2:
        mortar = (uint8_t)((tx == 0U) || (ty == 0U));
        accent = (uint8_t)((tx == 2U) || (tx == 13U));
        break;
    case 3:
        mortar = (uint8_t)(((tx + ty * 3U) & 7U) == 0U);
        break;
    default:
        mortar = (uint8_t)((tx == 0U) || (tx == 15U));
        accent = (uint8_t)((tx > 6U) && (tx < 9U));
        break;
    }
    return shade(accent ? p[1] : (mortar ? p[2] : p[0]), dark);
}

static void cast_rays(void)
{
    for (int x = 0; x < SCREEN_W; ++x) {
        float camera = 2.0f * (float)x / (float)SCREEN_W - 1.0f;
        float ray_x = dir_x + plane_x * camera;
        float ray_y = dir_y + plane_y * camera;
        int map_x = (int)pos_x, map_y = (int)pos_y;
        float delta_x = (ray_x == 0.0f) ? 1.0e30f : fabsf(1.0f / ray_x);
        float delta_y = (ray_y == 0.0f) ? 1.0e30f : fabsf(1.0f / ray_y);
        int step_x, step_y, side = 0;
        float side_x, side_y;
        if (ray_x < 0.0f) { step_x = -1; side_x = (pos_x - map_x) * delta_x; }
        else { step_x = 1; side_x = (map_x + 1.0f - pos_x) * delta_x; }
        if (ray_y < 0.0f) { step_y = -1; side_y = (pos_y - map_y) * delta_y; }
        else { step_y = 1; side_y = (map_y + 1.0f - pos_y) * delta_y; }

        while (world[map_y][map_x] == 0U) {
            if (side_x < side_y) { side_x += delta_x; map_x += step_x; side = 0; }
            else { side_y += delta_y; map_y += step_y; side = 1; }
        }
        float distance = side ? (side_y - delta_y) : (side_x - delta_x);
        if (distance < 0.05f) distance = 0.05f;
        int height = (int)((float)VIEW_H / distance);
        int top = VIEW_H / 2 - height / 2;
        int bottom = top + height;
        float hit = side ? pos_x + distance * ray_x : pos_y + distance * ray_y;
        hit -= floorf(hit);
        int tx = (int)(hit * (float)TEX_SIZE);
        if ((!side && ray_x > 0.0f) || (side && ray_y < 0.0f)) tx = TEX_SIZE - 1 - tx;
        columns[x].top = (int16_t)top;
        columns[x].bottom = (int16_t)bottom;
        columns[x].tex_x = (uint8_t)tx;
        columns[x].wall = world[map_y][map_x];
        columns[x].dark = (uint8_t)side;
    }
}

static void render_frame(void)
{
    cast_rays();
    ILI9341_Set_Address(0U, 0U, SCREEN_W - 1U, VIEW_H - 1U);
    ILI9341_Begin_Pixel_Stream();
    uint8_t buffer_index = 0U;
    for (int first_y = 0; first_y < VIEW_H; first_y += ROWS_PER_BUFFER) {
        int rows = VIEW_H - first_y;
        if (rows > ROWS_PER_BUFFER) rows = ROWS_PER_BUFFER;
        uint8_t *buffer = render_buffers[buffer_index];
        for (int row = 0; row < rows; ++row) {
            int y = first_y + row;
            uint8_t *output = buffer + row * SCREEN_W * 2;
            for (int x = 0; x < SCREEN_W; ++x) {
                RayColumn *c = &columns[x];
                uint16_t color;
                if (y < c->top) {
                    uint8_t band = (uint8_t)(y >> 4U);
                    color = (uint16_t)(0xDEFBU - ((uint16_t)band << 5U));
                } else if (y >= c->bottom) {
                    //uint8_t checker = (uint8_t)(((x >> 4U) ^ (y >> 4U)) & 1U);
                    //color = checker ? 0x18E3U : 0x1082U;
                    uint8_t band = (uint8_t)(y >> 4U);
                    color = (uint16_t)(0x0848U + ((uint16_t)band << 5U)); //0848
                } else {
                    int height = c->bottom - c->top;
                    int ty = ((y - c->top) * TEX_SIZE) / height;
                    color = texture_pixel(c->wall, c->tex_x, (uint8_t)ty, c->dark);
                }
                output[x * 2] = (uint8_t)(color >> 8U);
                output[x * 2 + 1] = (uint8_t)color;
            }
        }
        while (dma_busy) __WFI();
        dma_busy = 1U;
        if (HAL_SPI_Transmit_DMA(&hspi1, buffer,
                                 (uint16_t)(rows * SCREEN_W * 2)) != HAL_OK) {
            dma_busy = 0U;
            Error_Handler();
        }
        buffer_index ^= 1U;
    }
    while (dma_busy) __WFI();
    ILI9341_End_Pixel_Stream();
}

static void rotate(float sign)
{
    float s = ROT_SIN * sign;
    float old = dir_x;
    dir_x = dir_x * ROT_COS - dir_y * s;
    dir_y = old * s + dir_y * ROT_COS;
    old = plane_x;
    plane_x = plane_x * ROT_COS - plane_y * s;
    plane_y = old * s + plane_y * ROT_COS;
}

static void move(float sign)
{
    float nx = pos_x + dir_x * MOVE_SPEED * sign;
    float ny = pos_y + dir_y * MOVE_SPEED * sign;
    if (world[(int)pos_y][(int)nx] == 0U) pos_x = nx;
    if (world[(int)ny][(int)pos_x] == 0U) pos_y = ny;
}

int main(void)
{
    Raycaster_PlatformInit();
    ILI9341_Init();
    ILI9341_Fill_Screen(BLACK);
    draw_status_bar();
    draw_fps(0U);
    render_frame();
    uint32_t fps_started = HAL_GetTick();
    uint32_t frame_count = 1U;
    for (;;) {
        uint32_t keys = getKeyState();
        if (keys & (KBRD_BTN_2 | KBRD_BTN_MENU)) NVIC_SystemReset();
        if (keys & KBRD_BTN_UP) move(1.0f);
        if (keys & KBRD_BTN_DOWN) move(-1.0f);
        if (keys & KBRD_BTN_LEFT) rotate(-1.0f);
        if (keys & KBRD_BTN_RIGHT) rotate(1.0f);
        render_frame();
        ++frame_count;
        uint32_t now = HAL_GetTick();
        uint32_t elapsed = now - fps_started;
        if (elapsed >= 1000U) {
            draw_fps((frame_count * 1000U + elapsed / 2U) / elapsed);
            frame_count = 0U;
            fps_started = now;
        }
    }
}
