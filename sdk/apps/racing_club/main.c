#include "main.h"
#include "keyboard.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"
#include <math.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define VIEW_H 210
#define HORIZON 68
#define TRACK_W 32
#define TRACK_H 32
#define ROWS_PER_BUFFER 4
#define BUFFER_SIZE (SCREEN_W * 2 * ROWS_PER_BUFFER)
#define PI_F 3.14159265f

/* Two-dimensional track color map stored entirely in application Flash.
 * . grass, # asphalt, + curb, | center marking. */
static const char track[TRACK_H][TRACK_W + 1] = {
    "................................",
    "................................",
    "..........++++++++++++..........",
    "........++############++........",
    ".......+################+.......",
    "......+######||||||######+......",
    ".....+#####..........#####+.....",
    "....+#####............#####+....",
    "...+#####..............#####+...",
    "...+####................####+...",
    "..+#####................#####+..",
    "..+####..................####+..",
    ".+#####..................#####+.",
    ".+####....................####+.",
    ".+####....................####+.",
    ".+####....................####+.",
    ".+####....................####+.",
    ".+####....................####+.",
    ".+####....................####+.",
    ".+#####..................#####+.",
    "..+####..................####+..",
    "..+#####................#####+..",
    "...+####................####+...",
    "...+#####..............#####+...",
    "....+#####............#####+....",
    ".....+#####..........#####+.....",
    "......+######||||||######+......",
    ".......+################+.......",
    "........++############++........",
    "..........++++++++++++..........",
    "................................",
    "................................"
};

static uint8_t buffers[2][BUFFER_SIZE];
static volatile uint8_t dma_busy;
static float car_x = 16.0f, car_y = 27.5f, car_angle = -PI_F * 0.5f;
static float car_speed, steering_visual;

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi) { if (spi == &hspi1) dma_busy = 0U; }
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi) { if (spi == &hspi1) dma_busy = 0U; }

static char surface_at(float x, float y)
{
    int ix = (int)x, iy = (int)y;
    if (ix < 0 || ix >= TRACK_W || iy < 0 || iy >= TRACK_H) return '.';
    return track[iy][ix];
}

static uint16_t surface_color(char surface, int tx, int ty, float distance)
{
    uint16_t color;
    if (surface == '#') color = (((tx >> 1) ^ (ty >> 1)) & 1) ? 0x4A49U : 0x4228U;
    else if (surface == '+') color = ((tx + ty) & 1) ? 0xFFFFU : 0xD904U;
    else if (surface == '|') color = 0xFFE0U;
    else color = (((tx >> 1) ^ ty) & 1) ? 0x2CA4U : 0x2443U;
    if (distance > 14.0f) color = (uint16_t)(((color & 0xF7DEU) >> 1U) + 0x1082U);
    return color;
}

static uint8_t car_pixel(int x, int y, uint16_t *color)
{
    int cx = 160 + (int)(steering_visual * 7.0f);
    int local_x = x - cx, local_y = y - 169;
    if (local_y < 0 || local_y >= 36) return 0U;
    int half = 10 + local_y / 4;
    if (half > 19) half = 19;
    if (local_x < -half || local_x > half) return 0U;
    if ((local_x < -half + 4 || local_x > half - 4) && local_y > 22) *color = 0x0861U;
    else if (local_y < 11 && local_x > -7 && local_x < 7) *color = 0x2D5FU;
    else if ((local_x > -3 && local_x < 3) || local_y > 31) *color = 0xFFFFU;
    else *color = 0xF945U;
    return 1U;
}

static void render_frame(void)
{
    float dir_x = cosf(car_angle), dir_y = sinf(car_angle);
    float plane_x = -dir_y * 0.82f, plane_y = dir_x * 0.82f;
    float ray0_x = dir_x - plane_x, ray0_y = dir_y - plane_y;
    float ray1_x = dir_x + plane_x, ray1_y = dir_y + plane_y;
    ILI9341_Set_Address(0U, 0U, SCREEN_W - 1U, VIEW_H - 1U);
    ILI9341_Begin_Pixel_Stream();
    uint8_t index = 0U;
    for (int first_y = 0; first_y < VIEW_H; first_y += ROWS_PER_BUFFER) {
        int rows = VIEW_H - first_y; if (rows > ROWS_PER_BUFFER) rows = ROWS_PER_BUFFER;
        uint8_t *buffer = buffers[index];
        for (int row = 0; row < rows; ++row) {
            int y = first_y + row;
            uint8_t *out = buffer + row * SCREEN_W * 2;
            float distance = 0.0f, world_x = 0.0f, world_y = 0.0f, step_x = 0.0f, step_y = 0.0f;
            if (y >= HORIZON) {
                distance = 92.0f / (float)(y - HORIZON + 3);
                world_x = car_x + distance * ray0_x; world_y = car_y + distance * ray0_y;
                step_x = distance * (ray1_x - ray0_x) / SCREEN_W;
                step_y = distance * (ray1_y - ray0_y) / SCREEN_W;
            }
            for (int x = 0; x < SCREEN_W; ++x) {
                uint16_t color;
                if (y < HORIZON) {
                    color = (uint16_t)(0x18CDU + ((uint16_t)(y >> 3U) << 5U));
                    if (y > HORIZON - 5) color = 0xD69AU;
                } else {
                    int tx = (int)(world_x * 4.0f), ty = (int)(world_y * 4.0f);
                    color = surface_color(surface_at(world_x, world_y), tx, ty, distance);
                    world_x += step_x; world_y += step_y;
                }
                uint16_t car_color;
                if (car_pixel(x, y, &car_color)) color = car_color;
                out[x * 2] = (uint8_t)(color >> 8U); out[x * 2 + 1] = (uint8_t)color;
            }
        }
        while (dma_busy) __WFI();
        dma_busy = 1U;
        if (HAL_SPI_Transmit_DMA(&hspi1, buffer, (uint16_t)(rows * SCREEN_W * 2)) != HAL_OK) Error_Handler();
        index ^= 1U;
    }
    while (dma_busy) __WFI();
    ILI9341_End_Pixel_Stream();
}

static void draw_status(void)
{
    ILI9341_Draw_Rectangle(0U, VIEW_H, SCREEN_W, SCREEN_H - VIEW_H, 0x1082U);
    ILI9341_Draw_Horizontal_Line(0U, VIEW_H, SCREEN_W, 0xF945U);
    ILI9341_Draw_Text("RACING CLUB", 10U, 220U, WHITE, 1U, 0x1082U);
    ILI9341_Draw_Text("FPS", 144U, 220U, LIGHTGREY, 1U, 0x1082U);
    ILI9341_Draw_Text("KMH", 232U, 220U, LIGHTGREY, 1U, 0x1082U);
}

static void number_text(char out[4], uint32_t value)
{
    if (value > 999U) value = 999U;
    int n = 0;
    if (value >= 100U) out[n++] = (char)('0' + value / 100U);
    if (value >= 10U) out[n++] = (char)('0' + (value / 10U) % 10U);
    out[n++] = (char)('0' + value % 10U); out[n] = '\0';
}

static void update_status(uint32_t fps)
{
    char text[4];
    ILI9341_Draw_Rectangle(166U, 214U, 38U, 20U, 0x1082U);
    number_text(text, fps); ILI9341_Draw_Text(text, 170U, 220U, WHITE, 1U, 0x1082U);
    ILI9341_Draw_Rectangle(204U, 214U, 28U, 20U, 0x1082U);
    number_text(text, (uint32_t)(fabsf(car_speed) * 1250.0f));
    ILI9341_Draw_Text(text, 206U, 220U, 0xFFE0U, 1U, 0x1082U);
}

static void update_car(uint32_t keys)
{
    if (keys & KBRD_BTN_UP) car_speed += 0.0030f;
    else if (keys & KBRD_BTN_DOWN) car_speed -= 0.0040f;
    else car_speed *= 0.982f;
    if (car_speed > 0.115f) car_speed = 0.115f;
    if (car_speed < -0.045f) car_speed = -0.045f;
    float steer = 0.0f;
    if (keys & KBRD_BTN_LEFT) steer = -1.0f;
    if (keys & KBRD_BTN_RIGHT) steer = 1.0f;
    steering_visual += (steer - steering_visual) * 0.25f;
    if (fabsf(car_speed) > 0.004f) car_angle += steer * car_speed * 0.32f;
    float nx = car_x + cosf(car_angle) * car_speed;
    float ny = car_y + sinf(car_angle) * car_speed;
    car_x = nx; car_y = ny;
    char surface = surface_at(car_x, car_y);
    if (surface == '.') car_speed *= 0.91f; else car_speed *= 0.995f;
    if (car_x < 0.5f) car_x = 0.5f;
    if (car_x > TRACK_W - 0.5f) car_x = TRACK_W - 0.5f;
    if (car_y < 0.5f) car_y = 0.5f;
    if (car_y > TRACK_H - 0.5f) car_y = TRACK_H - 0.5f;
}

int main(void)
{
    RacingClub_PlatformInit(); ILI9341_Init(); ILI9341_Fill_Screen(BLACK);
    draw_status(); update_status(0U); render_frame();
    uint32_t started = HAL_GetTick(), frames = 1U;
    for (;;) {
        uint32_t keys = getKeyState();
        if (keys & (KBRD_BTN_2 | KBRD_BTN_MENU)) NVIC_SystemReset();
        update_car(keys); render_frame(); ++frames;
        uint32_t now = HAL_GetTick(), elapsed = now - started;
        if (elapsed >= 1000U) { update_status((frames * 1000U + elapsed / 2U) / elapsed); frames = 0U; started = now; }
    }
}
