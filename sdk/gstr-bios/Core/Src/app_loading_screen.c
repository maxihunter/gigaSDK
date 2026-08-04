#include "app_loading_screen.h"

#include <stddef.h>
#include <stdint.h>

#include "fonts/FreeSansBold18pt7b.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"

#define SCREEN_WIDTH       320U
#define SCREEN_HEIGHT      240U
#define LABEL_PANEL_X      62U
#define LABEL_PANEL_Y      86U
#define LABEL_PANEL_WIDTH  196U
#define LABEL_PANEL_HEIGHT 58U
#define LABEL_BASELINE     125U
#define LABEL_BACKGROUND   0x18CBU
#define LABEL_FOREGROUND   0xFFFFU

#define BAR_X              28U
#define BAR_Y              202U
#define BAR_WIDTH          264U
#define BAR_HEIGHT         18U
#define BAR_INNER_X        32U
#define BAR_INNER_Y        206U
#define BAR_INNER_WIDTH    256U
#define BAR_INNER_HEIGHT   10U
#define BAR_BORDER         0xBDF7U
#define BAR_TRACK          0x210CU
#define BAR_FILL           0x4E7FU
#define BAR_COMPLETE       0x5FE0U

typedef struct {
    uint16_t filled_pixels;
    uint8_t percent;
} LoadingScreenState;

static uint8_t gradient_line[SCREEN_WIDTH * 2U];

static uint16_t rgb565(uint16_t red, uint16_t green, uint16_t blue)
{
    return (uint16_t)(((red & 0xF8U) << 8U) |
                      ((green & 0xFCU) << 3U) | (blue >> 3U));
}

static uint16_t gradient_colour(uint16_t x, uint16_t y)
{
    /* Deep navy in the upper-left, muted violet in the lower-right. */
    uint32_t position = ((uint32_t)y * 3U) + x;
    const uint32_t span = ((SCREEN_HEIGHT - 1U) * 3U) + SCREEN_WIDTH - 1U;
    uint16_t red = (uint16_t)(7U + (47U * position) / span);
    uint16_t green = (uint16_t)(18U + (20U * position) / span);
    uint16_t blue = (uint16_t)(45U + (52U * position) / span);
    return rgb565(red, green, blue);
}

static void draw_gradient(void)
{
    ILI9341_Set_Address(0U, 0U, SCREEN_WIDTH - 1U, SCREEN_HEIGHT - 1U);
    ILI9341_Begin_Pixel_Stream();
    for (uint16_t y = 0U; y < SCREEN_HEIGHT; ++y) {
        for (uint16_t x = 0U; x < SCREEN_WIDTH; ++x) {
            uint16_t colour = gradient_colour(x, y);
            gradient_line[x * 2U] = (uint8_t)(colour >> 8U);
            gradient_line[(x * 2U) + 1U] = (uint8_t)colour;
        }
        ILI9341_Stream_Pixels(gradient_line, sizeof(gradient_line));
    }
    ILI9341_End_Pixel_Stream();
}

static uint16_t text_width(const char *text, const GFXfont *font)
{
    uint16_t width = 0U;
    while ((text != NULL) && (*text != '\0')) {
        uint8_t character = (uint8_t)*text++;
        if ((character >= font->first) && (character <= font->last))
            width += font->glyph[character - font->first].xAdvance;
    }
    return width;
}

static void draw_label(const char *text, uint16_t colour)
{
    uint16_t width = text_width(text, &FreeSansBold18pt7b);
    uint16_t x = width < SCREEN_WIDTH ? (SCREEN_WIDTH - width) / 2U : 0U;

    ILI9341_Draw_Rectangle(LABEL_PANEL_X, LABEL_PANEL_Y,
                           LABEL_PANEL_WIDTH, LABEL_PANEL_HEIGHT,
                           LABEL_BACKGROUND);
    ILI9341_Draw_Text_Font(text, x, LABEL_BASELINE, colour, 1U,
                           LABEL_BACKGROUND, &FreeSansBold18pt7b);
}

static void draw_progress_frame(void)
{
    ILI9341_Draw_Rectangle(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BORDER);
    ILI9341_Draw_Rectangle(BAR_X + 1U, BAR_Y + 1U,
                           BAR_WIDTH - 2U, BAR_HEIGHT - 2U, LABEL_BACKGROUND);
    ILI9341_Draw_Rectangle(BAR_INNER_X, BAR_INNER_Y,
                           BAR_INNER_WIDTH, BAR_INNER_HEIGHT, BAR_TRACK);
}

static uint8_t progress_percent(AppEngineProgressStage stage,
                                uint32_t completed, uint32_t total)
{
    uint32_t fraction = (total == 0U) ? 0U : (completed * 100U) / total;
    if (fraction > 100U) fraction = 100U;

    switch (stage) {
    case APP_ENGINE_PROGRESS_PREPARING:
        return 2U;
    case APP_ENGINE_PROGRESS_VALIDATING:
        return (uint8_t)(5U + (fraction * 20U) / 100U);
    case APP_ENGINE_PROGRESS_ERASING:
        return (uint8_t)(25U + (fraction * 20U) / 100U);
    case APP_ENGINE_PROGRESS_PROGRAMMING:
        return (uint8_t)(45U + (fraction * 47U) / 100U);
    case APP_ENGINE_PROGRESS_VERIFYING:
        return 96U;
    case APP_ENGINE_PROGRESS_COMPLETE:
        return 100U;
    default:
        return 0U;
    }
}

static void loading_progress(AppEngineProgressStage stage, uint32_t completed,
                             uint32_t total, void *context)
{
    LoadingScreenState *state = context;
    uint8_t percent = progress_percent(stage, completed, total);
    uint16_t pixels = (uint16_t)(((uint32_t)BAR_INNER_WIDTH * percent) / 100U);

    if (percent <= state->percent) return;
    if (percent == 100U) {
        ILI9341_Draw_Rectangle(BAR_INNER_X, BAR_INNER_Y,
                               BAR_INNER_WIDTH, BAR_INNER_HEIGHT,
                               BAR_COMPLETE);
        state->filled_pixels = BAR_INNER_WIDTH;
        state->percent = percent;
        return;
    }
    if (pixels > state->filled_pixels) {
        ILI9341_Draw_Rectangle(BAR_INNER_X + state->filled_pixels,
                               BAR_INNER_Y, pixels - state->filled_pixels,
                               BAR_INNER_HEIGHT, BAR_FILL);
        state->filled_pixels = pixels;
    }
    state->percent = percent;
}

__attribute__((section(".app_loading_api")))
AppEngineStatus AppLoadingScreen_Install(const char *app_id)
{
    LoadingScreenState state = {0};
    draw_gradient();
    ILI9341_Draw_Rectangle(LABEL_PANEL_X, LABEL_PANEL_Y,
                           LABEL_PANEL_WIDTH, LABEL_PANEL_HEIGHT,
                           LABEL_BACKGROUND);
    draw_label("Loading...", LABEL_FOREGROUND);
    draw_progress_frame();

    AppEngineStatus status = AppEngine_InstallWithProgress(
        app_id, loading_progress, &state);
    if (status != APP_ENGINE_OK)
        draw_label("Load failed", 0xF9C7U);
    return status;
}
