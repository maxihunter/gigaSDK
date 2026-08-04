#ifndef APP_CATALOG_H
#define APP_CATALOG_H

#include <stddef.h>

#include "app_engine.h"

#define APP_CATALOG_CAPACITY 32U

/* Rebuilds the in-memory catalog from /apps/<app_id>/app.bin. */
AppEngineStatus AppCatalog_Refresh(void);
void AppCatalog_Clear(void);
size_t AppCatalog_Count(void);
const AppEngineInfo *AppCatalog_Get(size_t index);
bool AppCatalog_WasTruncated(void);

#endif
