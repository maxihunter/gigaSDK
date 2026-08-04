#ifndef APP_ENGINE_H
#define APP_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_ENGINE_FORMAT_VERSION 1U
#define APP_ENGINE_ABI_VERSION    1U
#define APP_ENGINE_ICON_WIDTH     48U
#define APP_ENGINE_ICON_HEIGHT    48U
#define APP_ENGINE_ICON_PIXELS    (APP_ENGINE_ICON_WIDTH * APP_ENGINE_ICON_HEIGHT)
#define APP_ENGINE_ICON_SIZE      (APP_ENGINE_ICON_PIXELS * sizeof(uint16_t))
#define APP_ENGINE_ID_MAX         12U
#define APP_ENGINE_NAME_MAX       48U
#define APP_ENGINE_VERSION_MAX    16U
#define APP_ENGINE_FLASH_BASE     0x08020000UL
#define APP_ENGINE_FLASH_SIZE     (384UL * 1024UL)

typedef enum {
    APP_ENGINE_OK = 0,
    APP_ENGINE_END,
    APP_ENGINE_INVALID_ARGUMENT,
    APP_ENGINE_NOT_FOUND,
    APP_ENGINE_IO_ERROR,
    APP_ENGINE_BAD_FORMAT,
    APP_ENGINE_INCOMPATIBLE,
    APP_ENGINE_TOO_LARGE,
    APP_ENGINE_BAD_CRC,
    APP_ENGINE_BAD_VECTOR,
    APP_ENGINE_FLASH_ERROR
} AppEngineStatus;

typedef enum { APP_ENGINE_ICON_RGB565_LE = 1 } AppEngineIconFormat;

typedef struct {
    char app_id[APP_ENGINE_ID_MAX + 1U];
    char name[APP_ENGINE_NAME_MAX + 1U];
    char version[APP_ENGINE_VERSION_MAX + 1U];
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint16_t icon_width;
    uint16_t icon_height;
    uint16_t icon_format;
    bool installed;
} AppEngineInfo;

typedef bool (*AppEngineVisitor)(const AppEngineInfo *info, void *context);

typedef enum {
    APP_ENGINE_PROGRESS_PREPARING = 0,
    APP_ENGINE_PROGRESS_VALIDATING,
    APP_ENGINE_PROGRESS_ERASING,
    APP_ENGINE_PROGRESS_PROGRAMMING,
    APP_ENGINE_PROGRESS_VERIFYING,
    APP_ENGINE_PROGRESS_COMPLETE
} AppEngineProgressStage;

typedef void (*AppEngineProgressCallback)(AppEngineProgressStage stage,
                                          uint32_t completed,
                                          uint32_t total,
                                          void *context);

AppEngineStatus AppEngine_Scan(AppEngineVisitor visitor, void *context,
                               size_t *valid_count);
AppEngineStatus AppEngine_GetInfo(const char *app_id, AppEngineInfo *info);
AppEngineStatus AppEngine_ReadIcon(const char *app_id, uint16_t *rgb565,
                                   size_t pixel_count);
AppEngineStatus AppEngine_Install(const char *app_id);
AppEngineStatus AppEngine_InstallWithProgress(
    const char *app_id, AppEngineProgressCallback callback, void *context);
bool AppEngine_IsInstalled(const AppEngineInfo *info);
AppEngineStatus AppEngine_ValidateInstalled(void);
void AppEngine_LaunchInstalled(void) __attribute__((noreturn));
const char *AppEngine_StatusString(AppEngineStatus status);

#endif
