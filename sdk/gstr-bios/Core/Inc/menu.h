/*
 * This file is part of the gigaSDK source code.
 * Copyright (c) 2025 MaxiHunter
 */

#ifndef __MENU_H
#define __MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtc/rtc_clock.h"

#define MENU_MAX_DEPTH       8U
#define MENU_VISIBLE_ITEMS   8U

typedef struct Menu Menu;
typedef void (*MenuCallback)(void);

typedef enum {
    MENU_ITEM_SUBMENU,
    MENU_ITEM_INT,
    MENU_ITEM_BOOL,
    MENU_ITEM_ACTION,
    MENU_ITEM_APPLICATION,
    MENU_ITEM_INFO
} MenuItemType;

typedef struct {
    const char *label;
    MenuItemType type;
    union {
        const Menu *submenu;
        struct {
            int32_t *value;
            int32_t minimum;
            int32_t maximum;
            int32_t step;
        } integer;
        bool *boolean;
        MenuCallback callback;
        const char *info;
    } data;
} MenuItem;

struct Menu {
    const char *title;
    const MenuItem *items;
    size_t count;
};

#define MENU_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define MENU_SUBMENU(label_, menu_) \
    { (label_), MENU_ITEM_SUBMENU, { .submenu = (menu_) } }
#define MENU_INT(label_, value_, min_, max_, step_) \
    { (label_), MENU_ITEM_INT, \
      { .integer = { (value_), (min_), (max_), (step_) } } }
#define MENU_BOOL(label_, value_) \
    { (label_), MENU_ITEM_BOOL, { .boolean = (value_) } }
#define MENU_ACTION(label_, callback_) \
    { (label_), MENU_ITEM_ACTION, { .callback = (callback_) } }
#define MENU_INFO(label_, text_) \
    { (label_), MENU_ITEM_INFO, { .info = (text_) } }
/*
 * The callback of an application item must not return.  It takes ownership of
 * the display, input and main loop after it is called.
 */
#define MENU_APPLICATION(label_, callback_) \
    { (label_), MENU_ITEM_APPLICATION, { .callback = (callback_) } }

void MenuEngine_Init(const Menu *root);
void MenuEngine_Draw(void);
void MenuEngine_Up(void);
void MenuEngine_Down(void);
void MenuEngine_Left(void);
void MenuEngine_Right(void);
void MenuEngine_Select(void);
void MenuEngine_Back(void);
bool MenuEngine_IsEditing(void);

/* Default BIOS menu and compatibility entry points. */
void mainMenu_Init(MenuCallback application_callback);
void mainMenu_Handler(void);
void mainMenu_TriggerUp(void);
void mainMenu_TriggerDown(void);
void mainMenu_TriggerLeft(void);
void mainMenu_TriggerRight(void);
void mainMenu_TriggerSelect(void);
void mainMenu_TriggerBack(void);
uint8_t mainMenu_GetSelectedId(void);

void menuHeader_Handler(RTC_ClockDateTime *c_time, int batt);

#ifdef __cplusplus
}
#endif

#endif /* __MENU_H */
