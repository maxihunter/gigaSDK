/*
 * This file is part of the gigaSDK source code.
 * Copyright (c) 2025 MaxiHunter
 */

#include "menu.h"

#include <stdio.h>

#include "fonts/FreeSans9pt7b.h"
#include "ili9341/ILI9341_GFX.h"
#include "ili9341/ILI9341_STM32_Driver.h"

#define MENU_X0             30U
#define MENU_X1             290U
#define MENU_TITLE_BASELINE 34U
#define MENU_FIRST_ROW_Y    42U
#define MENU_ROW_HEIGHT     22U
#define MENU_TEXT_X         40U
#define MENU_VALUE_X        220U

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

static void application_dispatch(void)
{
    if (application_start != NULL) {
        application_start();
    }

    /* An application callback is not allowed to return to the BIOS menu. */
    for (;;) {
    }
}

static const Menu settings_menu;
static const Menu network_menu;
static const Menu storage_menu;
static const Menu media_menu;
static const Menu about_menu;

static const MenuItem settings_items[] = {
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
static const Menu network_menu = {
    "Network", network_items, MENU_ARRAY_SIZE(network_items)
};
static const Menu storage_menu = {
    "Storage", storage_items, MENU_ARRAY_SIZE(storage_items)
};
static const Menu media_menu = {
    "Media", media_items, MENU_ARRAY_SIZE(media_items)
};
static const Menu about_menu = {
    "gigaSDK BIOS", NULL, 0U
};

static const MenuItem root_items[] = {
    MENU_APPLICATION("APP", application_dispatch),
    MENU_SUBMENU("Network", &network_menu),
    MENU_SUBMENU("Storage", &storage_menu),
    MENU_SUBMENU("Media", &media_menu),
    MENU_SUBMENU("Settings", &settings_menu),
    MENU_SUBMENU("About", &about_menu),
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
        ILI9341_Draw_Filled_Rectangle_Coord(MENU_X0, 16U, MENU_X1, 224U,
                                             DARKGREY);
        ILI9341_Draw_Text_Font(menu->title, MENU_TEXT_X, MENU_TITLE_BASELINE,
                               WHITE, 1, DARKGREY, &FreeSans9pt7b);

        if (menu->count == 0U) {
            ILI9341_Draw_Text_Font("(empty)", MENU_TEXT_X,
                                   MENU_FIRST_ROW_Y + 16U, WHITE, 1,
                                   DARKGREY, &FreeSans9pt7b);
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
    keep_selection_visible();
    engine.dirty = true;
}

void MenuEngine_Down(void)
{
    const Menu *menu = current_menu();

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
    keep_selection_visible();
    engine.dirty = true;
}

void MenuEngine_Left(void)
{
    const MenuItem *item = current_item();

    if (engine.editing) {
        change_integer(item, -1);
    } else {
        MenuEngine_Back();
    }
}

void MenuEngine_Right(void)
{
    const MenuItem *item = current_item();

    if (engine.editing) {
        change_integer(item, 1);
    } else if ((item != NULL) && (item->type == MENU_ITEM_SUBMENU)) {
        MenuEngine_Select();
    }
}

void MenuEngine_Select(void)
{
    const MenuItem *item = current_item();

    if (item == NULL) {
        return;
    }

    switch (item->type) {
    case MENU_ITEM_SUBMENU:
        if ((item->data.submenu != NULL) &&
            (engine.depth + 1U < MENU_MAX_DEPTH)) {
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
    }
}

void MenuEngine_Back(void)
{
    if (engine.editing) {
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
