/*
 * This file is part of the gigaSDK source code.
 * Copyright (c) 2025 MaxiHunter
 */

#include "menu.h"

#include <stdio.h>

#include "fatfs.h"
#include "fonts/FreeSans9pt7b.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"
#include "main.h"
#include "app_catalog.h"
#include "app_loading_screen.h"
#include "audio/audio.h"

#define MENU_X0             30U
#define MENU_X1             290U
#define MENU_TITLE_BASELINE 34U
#define MENU_FIRST_ROW_Y    42U
#define MENU_ROW_HEIGHT     22U
#define MENU_TEXT_X         40U
#define MENU_VALUE_X        220U

#define APPS_GRADIENT_Y0    15U
#define APPS_GRADIENT_Y1    239U

/*
 * The APPs screen is composed one scanline at a time: the gradient, the cells,
 * the cursor and the icons all land in the same line buffer, which is then
 * pushed to the panel in a single burst.  Nothing is ever drawn over anything
 * else, so scrolling neither flickers nor shows a half-painted frame.
 */
#define APPS_COLUMNS        3U
#define APPS_TITLE_Y0       15
#define APPS_TITLE_Y1       37
#define APPS_TITLE_BASELINE 32U
#define APPS_VIEW_Y0        38
#define APPS_VIEW_Y1        239
#define APPS_VIEW_HEIGHT    (APPS_VIEW_Y1 - APPS_VIEW_Y0 + 1)
/* Whole rows that fit the viewport; scrolling rests on row boundaries. */
#define APPS_VISIBLE_ROWS   (APPS_VIEW_HEIGHT / APPS_CELL_Y_STEP)
#define APPS_CELL_WIDTH     90
#define APPS_CELL_HEIGHT    88
#define APPS_CELL_X0        10
#define APPS_CELL_X_STEP    105
#define APPS_CELL_Y_STEP    100
#define APPS_ROW_PAD        2
#define APPS_CURSOR_INSET   3
#define APPS_CURSOR_WIDTH   2
#define APPS_SCROLLBAR_X0   314
#define APPS_SCROLLBAR_X1   316
#define APPS_SCROLL_MIN     8
#define APPS_SCROLL_MAX     24

/* Every cell shares one colour; only the cursor outline marks the selection. */
#define APPS_CELL_COLOUR    0x320DU
#define APPS_CURSOR_COLOUR  WHITE
#define APPS_TRACK_COLOUR   0x2145U
#define APPS_THUMB_COLOUR   0x8410U

typedef struct {
    const Menu *menus[MENU_MAX_DEPTH];
    size_t selected[MENU_MAX_DEPTH];
    size_t scroll[MENU_MAX_DEPTH];
    uint8_t depth;
    bool editing;
    bool dirty;
    bool rendered_valid;
    const Menu *rendered_menu;
    size_t rendered_selected;
    size_t rendered_scroll;
} MenuEngineState;

static MenuEngineState engine;

static int32_t brightness = 50;
static int32_t volume = 75;
static int32_t sleep_timeout = 30;
static bool wifi_enabled = true;
static bool sound_enabled = true;
static bool storage_automount = true;
static MenuCallback application_start;
static int32_t rtc_year = 2026;
static int32_t rtc_month = 1;
static int32_t rtc_day = 1;
static int32_t rtc_hours = 0;
static int32_t rtc_minutes = 0;
static int32_t rtc_seconds = 0;
static char rtc_menu_title[24] = "Date & time";
static char status_bios_version[16];
static char status_sd_free[20];
static char status_battery[8];
static bool apps_active;
static bool apps_full_redraw;
static bool apps_cursor_visible = true;
static size_t apps_selected;
static size_t apps_top_row;
static int32_t apps_scroll;

static void menu_play_click(void)
{
    if (sound_enabled) {
        (void)Audio_PlayUiClick();
    }
}

static uint16_t apps_gradient_colour(uint16_t y)
{
    const uint16_t span = APPS_GRADIENT_Y1 - APPS_GRADIENT_Y0;
    uint16_t position = (y <= APPS_GRADIENT_Y0) ? 0U :
                        ((y >= APPS_GRADIENT_Y1) ? span :
                         (uint16_t)(y - APPS_GRADIENT_Y0));
    /*
     * Deep navy at the top, easing into indigo.  It stays dark on purpose so
     * the cells sitting on it read as raised tiles.
     */
    uint16_t red = (uint16_t)(1U + (3U * position) / span);
    uint16_t green = (uint16_t)(4U + (3U * position) / span);
    uint16_t blue = (uint16_t)(8U + (4U * position) / span);

    return (uint16_t)((red << 11U) | (green << 5U) | blue);
}

/* One scanline of the panel in the RGB565 byte order expected by the ILI9341. */
static uint8_t apps_line[ILI9341_SCREEN_WIDTH * 2U];
static uint16_t apps_icon_pixels[APP_ENGINE_ICON_PIXELS];

static void apps_line_span(int32_t x0, int32_t x1, uint16_t colour)
{
    uint8_t high = (uint8_t)(colour >> 8U);
    uint8_t low = (uint8_t)colour;

    if (x0 < 0) {
        x0 = 0;
    }
    if (x1 > (ILI9341_SCREEN_WIDTH - 1)) {
        x1 = ILI9341_SCREEN_WIDTH - 1;
    }
    for (int32_t x = x0; x <= x1; x++) {
        apps_line[x * 2] = high;
        apps_line[(x * 2) + 1] = low;
    }
}

/* Draws the shared menu background as one continuous RGB565 stream. */
static void apps_draw_gradient(void)
{
    ILI9341_Set_Address(0U, APPS_GRADIENT_Y0,
                        ILI9341_SCREEN_WIDTH - 1U, APPS_GRADIENT_Y1);
    ILI9341_Begin_Pixel_Stream();
    for (uint16_t y = APPS_GRADIENT_Y0; y <= APPS_GRADIENT_Y1; y++) {
        apps_line_span(0, ILI9341_SCREEN_WIDTH - 1,
                       apps_gradient_colour(y));
        ILI9341_Stream_Pixels(apps_line, (uint16_t)sizeof(apps_line));
    }
    ILI9341_End_Pixel_Stream();
}

static size_t apps_row_count(void)
{
    return (AppCatalog_Count() + APPS_COLUMNS - 1U) / APPS_COLUMNS;
}

static int32_t apps_content_height(void)
{
    return (int32_t)apps_row_count() * APPS_CELL_Y_STEP;
}

static int32_t apps_scroll_limit(void)
{
    int32_t limit = apps_content_height() - APPS_VIEW_HEIGHT;

    return (limit > 0) ? limit : 0;
}

/* Moves the first visible row the least amount needed to show the selection. */
static void apps_track_selection(void)
{
    size_t row = apps_selected / APPS_COLUMNS;
    size_t rows = apps_row_count();
    size_t maximum = (rows > APPS_VISIBLE_ROWS) ? (rows - APPS_VISIBLE_ROWS) : 0U;

    if (row < apps_top_row) {
        apps_top_row = row;
    } else if (row >= (apps_top_row + APPS_VISIBLE_ROWS)) {
        apps_top_row = row - (APPS_VISIBLE_ROWS - 1U);
    }
    if (apps_top_row > maximum) {
        apps_top_row = maximum;
    }
}

static int32_t apps_scroll_target(void)
{
    int32_t target = (int32_t)apps_top_row * APPS_CELL_Y_STEP;
    int32_t limit = apps_scroll_limit();

    return (target < limit) ? target : limit;
}

static void apps_compose_scrollbar(int32_t screen_y)
{
    int32_t content = apps_content_height();
    int32_t view_y = screen_y - APPS_VIEW_Y0;
    int32_t thumb_y;
    int32_t thumb_height;

    if (content <= APPS_VIEW_HEIGHT) {
        return;
    }
    thumb_height = (APPS_VIEW_HEIGHT * APPS_VIEW_HEIGHT) / content;
    thumb_y = (apps_scroll * APPS_VIEW_HEIGHT) / content;
    apps_line_span(APPS_SCROLLBAR_X0, APPS_SCROLLBAR_X1,
                   ((view_y >= thumb_y) && (view_y < (thumb_y + thumb_height))) ?
                   APPS_THUMB_COLOUR : APPS_TRACK_COLOUR);
}

/* Adds the part of one cell that falls on the scanline `dy` rows into it. */
static void apps_compose_cell(size_t index, int32_t x0, int32_t dy)
{
    int32_t x1 = x0 + APPS_CELL_WIDTH - 1;

    (void)index;

    apps_line_span(x0, x1, APPS_CELL_COLOUR);

    if (apps_cursor_visible && (index == apps_selected)) {
        int32_t inner = APPS_CURSOR_INSET + APPS_CURSOR_WIDTH;
        int32_t last = APPS_CELL_HEIGHT - 1 - APPS_CURSOR_INSET;

        if (((dy >= APPS_CURSOR_INSET) && (dy < inner)) ||
            ((dy <= last) && (dy > (last - APPS_CURSOR_WIDTH)))) {
            apps_line_span(x0 + APPS_CURSOR_INSET, x1 - APPS_CURSOR_INSET,
                           APPS_CURSOR_COLOUR);
        } else if ((dy >= inner) && (dy <= (last - APPS_CURSOR_WIDTH))) {
            apps_line_span(x0 + APPS_CURSOR_INSET, x0 + inner - 1,
                           APPS_CURSOR_COLOUR);
            apps_line_span(x1 - inner + 1, x1 - APPS_CURSOR_INSET,
                           APPS_CURSOR_COLOUR);
        }
    }

}

static void apps_compose_row(int32_t screen_y, int32_t content_y)
{
    int32_t row = content_y / APPS_CELL_Y_STEP;
    int32_t dy = (content_y % APPS_CELL_Y_STEP) - APPS_ROW_PAD;

    apps_line_span(0, ILI9341_SCREEN_WIDTH - 1,
                   apps_gradient_colour((uint16_t)screen_y));
    apps_compose_scrollbar(screen_y);

    if ((dy < 0) || (dy >= APPS_CELL_HEIGHT)) {
        return;
    }
    for (uint32_t column = 0U; column < APPS_COLUMNS; column++) {
        size_t index = ((size_t)row * APPS_COLUMNS) + column;

        if (index >= AppCatalog_Count()) {
            break;
        }
        apps_compose_cell(index, APPS_CELL_X0 + (int32_t)column * APPS_CELL_X_STEP,
                          dy);
    }
}

/* Reads and draws one icon at a time, reusing a single 4608-byte buffer. */
static void apps_draw_icons(int32_t redraw_y0, int32_t redraw_y1)
{
    size_t count = AppCatalog_Count();

    for (size_t index = 0U; index < count; ++index) {
        const AppEngineInfo *info = AppCatalog_Get(index);
        int32_t column = (int32_t)(index % APPS_COLUMNS);
        int32_t row = (int32_t)(index / APPS_COLUMNS);
        int32_t cell_y = row * APPS_CELL_Y_STEP + APPS_ROW_PAD - apps_scroll +
                         APPS_VIEW_Y0;
        int32_t icon_x = APPS_CELL_X0 + column * APPS_CELL_X_STEP +
                         (APPS_CELL_WIDTH - APP_ENGINE_ICON_WIDTH) / 2;
        int32_t icon_y = cell_y +
                         (APPS_CELL_HEIGHT - APP_ENGINE_ICON_HEIGHT) / 2;
        int32_t visible_y0 = icon_y;
        int32_t visible_y1 = icon_y + APP_ENGINE_ICON_HEIGHT - 1;

        if ((info == NULL) || (visible_y1 < redraw_y0) ||
            (visible_y0 > redraw_y1) || (visible_y1 < APPS_VIEW_Y0) ||
            (visible_y0 > APPS_VIEW_Y1)) {
            continue;
        }
        if (AppEngine_ReadIcon(info->app_id, apps_icon_pixels,
                               APP_ENGINE_ICON_PIXELS) != APP_ENGINE_OK) {
            continue;
        }
        if (visible_y0 < APPS_VIEW_Y0) visible_y0 = APPS_VIEW_Y0;
        if (visible_y0 < redraw_y0) visible_y0 = redraw_y0;
        if (visible_y1 > APPS_VIEW_Y1) visible_y1 = APPS_VIEW_Y1;
        if (visible_y1 > redraw_y1) visible_y1 = redraw_y1;

        ILI9341_Set_Address((uint16_t)icon_x, (uint16_t)visible_y0,
                            (uint16_t)(icon_x + APP_ENGINE_ICON_WIDTH - 1),
                            (uint16_t)visible_y1);
        ILI9341_Begin_Pixel_Stream();
        for (int32_t y = visible_y0; y <= visible_y1; ++y) {
            size_t source_row = (size_t)(y - icon_y);
            const uint16_t *pixels = &apps_icon_pixels[
                source_row * APP_ENGINE_ICON_WIDTH];
            for (size_t x = 0U; x < APP_ENGINE_ICON_WIDTH; ++x) {
                apps_line[x * 2U] = (uint8_t)(pixels[x] >> 8U);
                apps_line[x * 2U + 1U] = (uint8_t)pixels[x];
            }
            ILI9341_Stream_Pixels(apps_line, APP_ENGINE_ICON_WIDTH * 2U);
        }
        ILI9341_End_Pixel_Stream();
    }
}

/*
 * Repaints the screen rows y0..y1 of the grid.  The address window is opened
 * once and every row is streamed as a single burst, so each pixel is written
 * exactly once per frame.
 */
static void apps_render(int32_t y0, int32_t y1)
{
    if (y0 < APPS_VIEW_Y0) {
        y0 = APPS_VIEW_Y0;
    }
    if (y1 > APPS_VIEW_Y1) {
        y1 = APPS_VIEW_Y1;
    }
    if (y0 > y1) {
        return;
    }

    ILI9341_Set_Address((uint16_t)0U, (uint16_t)y0,
                        (uint16_t)(ILI9341_SCREEN_WIDTH - 1), (uint16_t)y1);
    ILI9341_Begin_Pixel_Stream();
    for (int32_t y = y0; y <= y1; y++) {
        apps_compose_row(y, (y - APPS_VIEW_Y0) + apps_scroll);
        ILI9341_Stream_Pixels(apps_line, (uint16_t)sizeof(apps_line));
    }
    ILI9341_End_Pixel_Stream();
}

/* Screen rows occupied by the row of cells holding `index`. */
static void apps_row_bounds(size_t index, int32_t *y0, int32_t *y1)
{
    *y0 = ((int32_t)(index / APPS_COLUMNS) * APPS_CELL_Y_STEP) + APPS_ROW_PAD -
          apps_scroll + APPS_VIEW_Y0;
    *y1 = *y0 + APPS_CELL_HEIGHT - 1;
}

static uint16_t apps_text_width(const char *text, const GFXfont *font)
{
    uint16_t width = 0U;

    for (; *text != '\0'; text++) {
        uint8_t character = (uint8_t)*text;

        if ((character >= font->first) && (character <= font->last)) {
            width += font->glyph[character - font->first].xAdvance;
        }
    }
    return width;
}

/* The name of the selected application replaces the former "APPs" caption. */
static void apps_draw_title(void)
{
    const AppEngineInfo *selected = AppCatalog_Get(apps_selected);
    const char *name = (selected != NULL) ? selected->name : "No applications";
    uint16_t background =
        apps_gradient_colour((APPS_TITLE_Y0 + APPS_TITLE_Y1) / 2U);
    uint16_t width = apps_text_width(name, &FreeSans9pt7b);
    uint16_t x = (width < ILI9341_SCREEN_WIDTH) ?
                 (uint16_t)((ILI9341_SCREEN_WIDTH - width) / 2U) : 0U;

    /* The second corner of a filled rectangle is exclusive in the GFX layer. */
    ILI9341_Draw_Filled_Rectangle_Coord(0U, APPS_TITLE_Y0,
                                         ILI9341_SCREEN_WIDTH,
                                         APPS_TITLE_Y1 + 1, background);
    ILI9341_Draw_Text_Font(name, x, APPS_TITLE_BASELINE, WHITE, 1U,
                           background, &FreeSans9pt7b);
}

static void apps_draw(void)
{
    if (!apps_active || !apps_full_redraw) {
        return;
    }
    apps_draw_title();
    apps_render(APPS_VIEW_Y0, APPS_VIEW_Y1);
    apps_draw_icons(APPS_VIEW_Y0, APPS_VIEW_Y1);
    apps_full_redraw = false;
}

/*
 * Slides the grid to `target`, decelerating towards the end.  The pace is set
 * by the time a frame needs on the bus, so no artificial delay is required.
 */
static void apps_scroll_animate(int32_t target)
{
    while (apps_scroll != target) {
        int32_t remaining = target - apps_scroll;
        int32_t distance = (remaining > 0) ? remaining : -remaining;
        int32_t step = (distance + 2) / 3;

        if (step < APPS_SCROLL_MIN) {
            step = APPS_SCROLL_MIN;
        }
        if (step > APPS_SCROLL_MAX) {
            step = APPS_SCROLL_MAX;
        }
        if (step > distance) {
            step = distance;
        }
        apps_scroll += (remaining > 0) ? step : -step;
        apps_render(APPS_VIEW_Y0, APPS_VIEW_Y1);
    }
    apps_draw_icons(APPS_VIEW_Y0, APPS_VIEW_Y1);
}

static void apps_open(void)
{
    apps_active = true;
    apps_cursor_visible = true;
    apps_selected = 0U;
    apps_top_row = 0U;
    apps_scroll = 0;
    apps_full_redraw = true;
}

static void apps_move_selection(size_t new_selected)
{
    size_t old_selected = apps_selected;
    int32_t target;
    int32_t old_y0;
    int32_t old_y1;
    int32_t new_y0;
    int32_t new_y1;

    if ((new_selected >= AppCatalog_Count()) ||
        (new_selected == old_selected)) {
        return;
    }
    menu_play_click();
    apps_selected = new_selected;
    apps_track_selection();
    apps_draw_title();

    target = apps_scroll_target();
    if (target != apps_scroll) {
        apps_scroll_animate(target);
        return;
    }
    /* Without scrolling only the cell rows losing and gaining the cursor move. */
    apps_row_bounds(old_selected, &old_y0, &old_y1);
    apps_row_bounds(new_selected, &new_y0, &new_y1);
    apps_render((old_y0 < new_y0) ? old_y0 : new_y0,
                (old_y1 > new_y1) ? old_y1 : new_y1);
    apps_draw_icons((old_y0 < new_y0) ? old_y0 : new_y0,
                    (old_y1 > new_y1) ? old_y1 : new_y1);
}

static void apps_flash_selection(void)
{
    int32_t y0;
    int32_t y1;

    apps_row_bounds(apps_selected, &y0, &y1);
    apps_cursor_visible = false;
    apps_render(y0, y1);
    apps_draw_icons(y0, y1);
    HAL_Delay(70U);
    apps_cursor_visible = true;
    apps_render(y0, y1);
    apps_draw_icons(y0, y1);
}

static const Menu settings_menu;
static const Menu rtc_settings_menu;
static const Menu network_menu;
static const Menu storage_menu;
static const Menu media_menu;
static const Menu status_menu;

static void status_format_size(char *buffer, size_t size, uint64_t bytes)
{
    const uint64_t gib = 1024ULL * 1024ULL * 1024ULL;
    const uint64_t mib = 1024ULL * 1024ULL;

    if (bytes >= gib) {
        uint64_t tenths = (bytes * 10ULL) / gib;
        snprintf(buffer, size, "%lu.%lu GB",
                 (unsigned long)(tenths / 10ULL),
                 (unsigned long)(tenths % 10ULL));
    } else {
        uint64_t tenths = (bytes * 10ULL) / mib;
        snprintf(buffer, size, "%lu.%lu MB",
                 (unsigned long)(tenths / 10ULL),
                 (unsigned long)(tenths % 10ULL));
    }
}

static void status_refresh(void)
{
    DWORD free_clusters;
    FATFS *filesystem;

    snprintf(status_bios_version, sizeof(status_bios_version), "%s",
             BIOS_VERSION);
    snprintf(status_battery, sizeof(status_battery), "%u%%", 75U);

    if (f_getfree(SDPath, &free_clusters, &filesystem) == FR_OK) {
        uint64_t free_bytes = (uint64_t)free_clusters *
                              (uint64_t)filesystem->csize * 512ULL;
        status_format_size(status_sd_free, sizeof(status_sd_free), free_bytes);
    } else {
        snprintf(status_sd_free, sizeof(status_sd_free), "unavailable");
    }
}

static void rtc_editor_read(void)
{
    RTC_ClockDateTime date_time;

    if (RTC_Clock_Get(&date_time) == HAL_OK) {
        rtc_year = date_time.year;
        rtc_month = date_time.month;
        rtc_day = date_time.day;
        rtc_hours = date_time.hours;
        rtc_minutes = date_time.minutes;
        rtc_seconds = date_time.seconds;
        snprintf(rtc_menu_title, sizeof(rtc_menu_title), "Date & time");
    } else {
        snprintf(rtc_menu_title, sizeof(rtc_menu_title), "Date/time: read err");
    }
}

static void rtc_editor_apply(void)
{
    HAL_StatusTypeDef status = RTC_Clock_Set(
        (uint16_t)rtc_year, (uint8_t)rtc_month, (uint8_t)rtc_day,
        (uint8_t)rtc_hours, (uint8_t)rtc_minutes, (uint8_t)rtc_seconds);

    snprintf(rtc_menu_title, sizeof(rtc_menu_title),
             (status == HAL_OK) ? "Date/time: saved" : "Date/time: invalid");
}

static const MenuItem rtc_settings_items[] = {
    MENU_INT("Year", &rtc_year, 2000, 2099, 1),
    MENU_INT("Month", &rtc_month, 1, 12, 1),
    MENU_INT("Day", &rtc_day, 1, 31, 1),
    MENU_INT("Hour", &rtc_hours, 0, 23, 1),
    MENU_INT("Minute", &rtc_minutes, 0, 59, 1),
    MENU_INT("Second", &rtc_seconds, 0, 59, 1),
    MENU_ACTION("Read RTC", rtc_editor_read),
    MENU_ACTION("Apply", rtc_editor_apply),
};

static const MenuItem settings_items[] = {
    MENU_SUBMENU("Date & time", &rtc_settings_menu),
    MENU_INT("Brightness", &brightness, 0, 100, 5),
    MENU_INT("Sleep, sec", &sleep_timeout, 0, 300, 10),
};

static const MenuItem network_items[] = {
    MENU_BOOL("Wi-Fi", &wifi_enabled),
};

static const MenuItem media_items[] = {
    MENU_BOOL("Sound", &sound_enabled),
    MENU_INT("Volume", &volume, 0, 100, 5),
};

static const MenuItem storage_items[] = {
    MENU_BOOL("Auto mount", &storage_automount),
};

static const Menu settings_menu = {
    "Settings", settings_items, MENU_ARRAY_SIZE(settings_items)
};
static const Menu rtc_settings_menu = {
    rtc_menu_title, rtc_settings_items, MENU_ARRAY_SIZE(rtc_settings_items)
};
static const Menu network_menu = {
    "Network", network_items, MENU_ARRAY_SIZE(network_items)
};
static const Menu storage_menu = {
    "Storage", storage_items, MENU_ARRAY_SIZE(storage_items)
};
static const Menu media_menu = {
    "Media", media_items, MENU_ARRAY_SIZE(media_items)
};
static const MenuItem status_items[] = {
    MENU_INFO("BIOS version", status_bios_version),
    MENU_INFO("SD free", status_sd_free),
    MENU_INFO("Battery", status_battery),
};
static const Menu status_menu = {
    "Status", status_items, MENU_ARRAY_SIZE(status_items)
};

static const MenuItem root_items[] = {
    MENU_ACTION("APPs", apps_open),
    MENU_SUBMENU("Network", &network_menu),
    MENU_SUBMENU("Storage", &storage_menu),
    MENU_SUBMENU("Settings", &settings_menu),
    MENU_SUBMENU("Status", &status_menu),
};

static const Menu root_menu = {
    "Main menu", root_items, MENU_ARRAY_SIZE(root_items)
};

static const Menu *current_menu(void)
{
    return engine.menus[engine.depth];
}

static const MenuItem *current_item(void)
{
    const Menu *menu = current_menu();

    if ((menu == NULL) || (menu->count == 0U)) {
        return NULL;
    }
    return &menu->items[engine.selected[engine.depth]];
}

static void keep_selection_visible(void)
{
    size_t selected = engine.selected[engine.depth];
    size_t *scroll = &engine.scroll[engine.depth];

    if (selected < *scroll) {
        *scroll = selected;
    } else if (selected >= (*scroll + MENU_VISIBLE_ITEMS)) {
        *scroll = selected - MENU_VISIBLE_ITEMS + 1U;
    }
}

static void change_integer(const MenuItem *item, int direction)
{
    int32_t value;
    int32_t step;
    int64_t candidate;

    if ((item == NULL) || (item->type != MENU_ITEM_INT) ||
        (item->data.integer.value == NULL)) {
        return;
    }

    value = *item->data.integer.value;
    step = item->data.integer.step;
    if (step <= 0) {
        step = 1;
    }

    candidate = (int64_t)value + ((direction > 0) ? step : -step);
    if (candidate > item->data.integer.maximum) {
        value = item->data.integer.maximum;
    } else if (candidate < item->data.integer.minimum) {
        value = item->data.integer.minimum;
    } else {
        value = (int32_t)candidate;
    }

    *item->data.integer.value = value;
    engine.dirty = true;
}

static void draw_item_value(const MenuItem *item, uint16_t baseline,
                            uint16_t foreground, uint16_t background)
{
    char value[16];

    switch (item->type) {
    case MENU_ITEM_SUBMENU:
        ILI9341_Draw_Text_Font(">", MENU_VALUE_X + 45U, baseline,
                               foreground, 1, background, &FreeSans9pt7b);
        break;
    case MENU_ITEM_INT:
        if (item->data.integer.value != NULL) {
            snprintf(value, sizeof(value), "%ld",
                     (long)*item->data.integer.value);
            ILI9341_Draw_Text_Font(value, MENU_VALUE_X, baseline,
                                   foreground, 1, background, &FreeSans9pt7b);
        }
        break;
    case MENU_ITEM_BOOL:
        ILI9341_Draw_Text_Font(
            (item->data.boolean != NULL && *item->data.boolean) ? "ON" : "OFF",
            MENU_VALUE_X, baseline, foreground, 1, background, &FreeSans9pt7b);
        break;
    case MENU_ITEM_ACTION:
        ILI9341_Draw_Text_Font("RUN", MENU_VALUE_X, baseline,
                               foreground, 1, background, &FreeSans9pt7b);
        break;
    case MENU_ITEM_APPLICATION:
        ILI9341_Draw_Text_Font("START", MENU_VALUE_X, baseline,
                               foreground, 1, background, &FreeSans9pt7b);
        break;
    case MENU_ITEM_INFO:
        if (item->data.info != NULL) {
            ILI9341_Draw_Text_Font(item->data.info, MENU_VALUE_X - 45U,
                                   baseline, foreground, 1, background,
                                   &FreeSans9pt7b);
        }
        break;
    }
}

static void draw_menu_item(const Menu *menu, size_t index, size_t first)
{
    uint16_t row = (uint16_t)(index - first);
    uint16_t y0 = MENU_FIRST_ROW_Y + row * MENU_ROW_HEIGHT;
    uint16_t baseline = y0 + 16U;
    bool selected = (index == engine.selected[engine.depth]);
    uint16_t background = selected ? YELLOW : DARKGREY;
    uint16_t foreground = selected ? BLACK : WHITE;

    ILI9341_Draw_Filled_Rectangle_Coord(MENU_X0, y0, MENU_X1,
                                         y0 + MENU_ROW_HEIGHT - 1U,
                                         background);
    if (selected && engine.editing) {
        ILI9341_Draw_Text_Font("*", MENU_X0 + 3U, baseline,
                               foreground, 1, background, &FreeSans9pt7b);
    }
    ILI9341_Draw_Text_Font(menu->items[index].label, MENU_TEXT_X, baseline,
                           foreground, 1, background, &FreeSans9pt7b);
    draw_item_value(&menu->items[index], baseline, foreground, background);
}

void MenuEngine_Init(const Menu *root)
{
    engine.depth = 0U;
    engine.editing = false;
    engine.dirty = true;
    engine.rendered_valid = false;
    engine.rendered_menu = NULL;
    engine.rendered_selected = 0U;
    engine.rendered_scroll = 0U;

    for (uint8_t i = 0; i < MENU_MAX_DEPTH; i++) {
        engine.menus[i] = NULL;
        engine.selected[i] = 0U;
        engine.scroll[i] = 0U;
    }
    engine.menus[0] = root;
}

void MenuEngine_Draw(void)
{
    const Menu *menu = current_menu();
    size_t first;
    size_t last;
    size_t selected;
    bool full_redraw;

    if (apps_active) {
        apps_draw();
        return;
    }
    if (!engine.dirty || (menu == NULL)) {
        return;
    }

    first = engine.scroll[engine.depth];
    selected = engine.selected[engine.depth];
    full_redraw = !engine.rendered_valid ||
                  (engine.rendered_menu != menu) ||
                  (engine.rendered_scroll != first);

    last = first + MENU_VISIBLE_ITEMS;
    if (last > menu->count) {
        last = menu->count;
    }

    if (full_redraw) {
        apps_draw_gradient();
        ILI9341_Draw_Text_Font(menu->title, MENU_TEXT_X, MENU_TITLE_BASELINE,
                               WHITE, 1, apps_gradient_colour(MENU_TITLE_BASELINE),
                               &FreeSans9pt7b);

        if (menu->count == 0U) {
            ILI9341_Draw_Text_Font("(empty)", MENU_TEXT_X,
                                   MENU_FIRST_ROW_Y + 16U, WHITE, 1,
                                   apps_gradient_colour(MENU_FIRST_ROW_Y + 8U),
                                   &FreeSans9pt7b);
        }
        for (size_t i = first; i < last; i++) {
            draw_menu_item(menu, i, first);
        }
    } else {
        /*
         * With an unchanged menu and scroll window only the old and new
         * selected rows can differ.  A value/edit-state change redraws the
         * selected row once.
         */
        if ((engine.rendered_selected != selected) &&
            (engine.rendered_selected >= first) &&
            (engine.rendered_selected < last)) {
            draw_menu_item(menu, engine.rendered_selected, first);
        }
        if ((selected >= first) && (selected < last)) {
            draw_menu_item(menu, selected, first);
        }
    }

    engine.rendered_valid = true;
    engine.rendered_menu = menu;
    engine.rendered_selected = selected;
    engine.rendered_scroll = first;
    engine.dirty = false;
}

void MenuEngine_Up(void)
{
    const Menu *menu = current_menu();

    if (apps_active) {
        if (apps_selected >= APPS_COLUMNS) {
            apps_move_selection(apps_selected - APPS_COLUMNS);
        }
        return;
    }
    if ((menu == NULL) || (menu->count == 0U)) {
        return;
    }
    if (engine.editing) {
        change_integer(current_item(), 1);
        return;
    }

    if (engine.selected[engine.depth] == 0U) {
        engine.selected[engine.depth] = menu->count - 1U;
    } else {
        engine.selected[engine.depth]--;
    }
    menu_play_click();
    keep_selection_visible();
    engine.dirty = true;
}

void MenuEngine_Down(void)
{
    const Menu *menu = current_menu();

    if (apps_active) {
        size_t count = AppCatalog_Count();
        size_t target = apps_selected + APPS_COLUMNS;

        if (count == 0U) {
            return;
        }
        if (target >= count) {
            /* A partly filled last row is entered at its final entry. */
            if ((apps_selected / APPS_COLUMNS) >= ((count - 1U) / APPS_COLUMNS)) {
                return;
            }
            target = count - 1U;
        }
        apps_move_selection(target);
        return;
    }
    if ((menu == NULL) || (menu->count == 0U)) {
        return;
    }
    if (engine.editing) {
        change_integer(current_item(), -1);
        return;
    }

    engine.selected[engine.depth]++;
    if (engine.selected[engine.depth] >= menu->count) {
        engine.selected[engine.depth] = 0U;
    }
    menu_play_click();
    keep_selection_visible();
    engine.dirty = true;
}

void MenuEngine_Left(void)
{
    const MenuItem *item = current_item();

    if (apps_active) {
        if ((apps_selected % APPS_COLUMNS) > 0U) {
            apps_move_selection(apps_selected - 1U);
        }
        return;
    }
    if (engine.editing) {
        change_integer(item, -1);
    } else {
        MenuEngine_Back();
    }
}

void MenuEngine_Right(void)
{
    const MenuItem *item = current_item();

    if (apps_active) {
        if ((((apps_selected % APPS_COLUMNS) + 1U) < APPS_COLUMNS) &&
            ((apps_selected + 1U) < AppCatalog_Count())) {
            apps_move_selection(apps_selected + 1U);
        }
        return;
    }
    if (engine.editing) {
        change_integer(item, 1);
    } else if ((item != NULL) && (item->type == MENU_ITEM_SUBMENU)) {
        MenuEngine_Select();
    }
}

void MenuEngine_Select(void)
{
    const MenuItem *item = current_item();

    if (apps_active) {
        const AppEngineInfo *selected = AppCatalog_Get(apps_selected);

        if (selected == NULL) {
            return;
        }
        apps_flash_selection();
        AppEngineStatus status = AppLoadingScreen_Install(selected->app_id);
        if (status == APP_ENGINE_OK) {
            AppEngine_LaunchInstalled();
        }
        apps_full_redraw = true;
        return;
    }
    if (item == NULL) {
        return;
    }

    switch (item->type) {
    case MENU_ITEM_SUBMENU:
        if ((item->data.submenu != NULL) &&
            (engine.depth + 1U < MENU_MAX_DEPTH)) {
            if (item->data.submenu == &status_menu) {
                status_refresh();
            }
            engine.depth++;
            engine.menus[engine.depth] = item->data.submenu;
            engine.selected[engine.depth] = 0U;
            engine.scroll[engine.depth] = 0U;
            engine.editing = false;
            engine.dirty = true;
        }
        break;
    case MENU_ITEM_INT:
        engine.editing = !engine.editing;
        engine.dirty = true;
        break;
    case MENU_ITEM_BOOL:
        if (item->data.boolean != NULL) {
            *item->data.boolean = !*item->data.boolean;
            engine.dirty = true;
        }
        break;
    case MENU_ITEM_ACTION:
        if (item->data.callback != NULL) {
            item->data.callback();
            engine.rendered_valid = false;
            engine.dirty = true;
        }
        break;
    case MENU_ITEM_APPLICATION:
        if (item->data.callback != NULL) {
            item->data.callback();
        }
        for (;;) {
        }
    case MENU_ITEM_INFO:
        break;
    }
}

void MenuEngine_Back(void)
{
    if (apps_active) {
        apps_active = false;
        engine.rendered_valid = false;
        engine.dirty = true;
    } else if (engine.editing) {
        engine.editing = false;
        engine.dirty = true;
    } else if (engine.depth > 0U) {
        engine.menus[engine.depth] = NULL;
        engine.depth--;
        engine.dirty = true;
    }
}

bool MenuEngine_IsEditing(void)
{
    return engine.editing;
}

void mainMenu_Init(MenuCallback application_callback)
{
    application_start = application_callback;
    (void)application_start;
    apps_active = false;
    rtc_editor_read();
    MenuEngine_Init(&root_menu);
}

void mainMenu_Handler(void)       { MenuEngine_Draw(); }
void mainMenu_TriggerUp(void)     { MenuEngine_Up(); }
void mainMenu_TriggerDown(void)   { MenuEngine_Down(); }
void mainMenu_TriggerLeft(void)   { MenuEngine_Left(); }
void mainMenu_TriggerRight(void)  { MenuEngine_Right(); }
void mainMenu_TriggerSelect(void) { MenuEngine_Select(); }
void mainMenu_TriggerBack(void)   { MenuEngine_Back(); }

uint8_t mainMenu_GetSelectedId(void)
{
    return (uint8_t)engine.selected[engine.depth];
}

static void menuHeader_DrawBattery(int level)
{
    if (level < 0) {
        level = 0;
    } else if (level > 4) {
        level = 4;
    }

    ILI9341_Draw_Hollow_Rectangle_Coord(291U, 2U, 316U, 12U, WHITE);
    ILI9341_Draw_Filled_Rectangle_Coord(317U, 5U, 319U, 9U, WHITE);
    for (uint8_t segment = 0; segment < 4U; segment++) {
        uint16_t x0 = 295U + segment * 5U;
        uint16_t colour = (segment < level) ? WHITE : BLACK;
        ILI9341_Draw_Filled_Rectangle_Coord(x0, 5U, x0 + 3U, 11U, colour);
    }
}

void menuHeader_Handler(RTC_ClockDateTime *c_time, int batt)
{
    char time[10];

    if (c_time == NULL) {
        return;
    }

    ILI9341_Draw_Filled_Rectangle_Coord(0U, 0U, 319U, 14U, BLACK);
    snprintf(time, sizeof(time), "%02u:%02u", c_time->hours, c_time->minutes);
    ILI9341_Draw_Text_Font(time, 10U, 12U, WHITE, 1U, BLACK, &FreeSans9pt7b);
    menuHeader_DrawBattery(batt);
}
