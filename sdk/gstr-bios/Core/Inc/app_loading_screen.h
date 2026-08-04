#ifndef APP_LOADING_SCREEN_H
#define APP_LOADING_SCREEN_H

#include "app_engine.h"

/* Draws the loading screen and installs /apps/<app_id>/app.bin. */
AppEngineStatus AppLoadingScreen_Install(const char *app_id);

#endif
