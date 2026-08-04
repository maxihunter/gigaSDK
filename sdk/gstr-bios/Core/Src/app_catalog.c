#include "app_catalog.h"

#include <string.h>

static AppEngineInfo entries[APP_CATALOG_CAPACITY];
static size_t entry_count;
static bool truncated;

static int compare_entries(const AppEngineInfo *left,
                           const AppEngineInfo *right)
{
    int result = strcmp(left->name, right->name);
    return (result != 0) ? result : strcmp(left->app_id, right->app_id);
}

static bool collect_entry(const AppEngineInfo *info, void *context)
{
    (void)context;
    if (entry_count >= APP_CATALOG_CAPACITY) {
        truncated = true;
        return false;
    }
    entries[entry_count++] = *info;
    return true;
}

static void sort_entries(void)
{
    for (size_t index = 1U; index < entry_count; ++index) {
        AppEngineInfo current = entries[index];
        size_t position = index;
        while ((position > 0U) &&
               (compare_entries(&current, &entries[position - 1U]) < 0)) {
            entries[position] = entries[position - 1U];
            --position;
        }
        entries[position] = current;
    }
}

AppEngineStatus AppCatalog_Refresh(void)
{
    size_t scanned = 0U;
    AppCatalog_Clear();
    AppEngineStatus status = AppEngine_Scan(collect_entry, NULL, &scanned);
    if (status == APP_ENGINE_NOT_FOUND) return APP_ENGINE_OK;
    if (status != APP_ENGINE_OK) {
        AppCatalog_Clear();
        return status;
    }
    sort_entries();
    return APP_ENGINE_OK;
}

void AppCatalog_Clear(void)
{
    entry_count = 0U;
    truncated = false;
}

size_t AppCatalog_Count(void)
{
    return entry_count;
}

const AppEngineInfo *AppCatalog_Get(size_t index)
{
    return (index < entry_count) ? &entries[index] : NULL;
}

bool AppCatalog_WasTruncated(void)
{
    return truncated;
}
