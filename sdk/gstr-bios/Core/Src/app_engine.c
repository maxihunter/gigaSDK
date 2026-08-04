#include "app_engine.h"

#include <stdio.h>
#include <string.h>

#include "fatfs.h"
#include "stm32f4xx_hal.h"

#define APP_IMAGE_MAGIC          0x50504147UL
#define APP_IMAGE_HEADER_SIZE    256U
#define APP_IMAGE_ICON_OFFSET    APP_IMAGE_HEADER_SIZE
#define APP_IMAGE_PAYLOAD_OFFSET (APP_IMAGE_ICON_OFFSET + APP_ENGINE_ICON_SIZE)
#define APP_IMAGE_TARGET_FLASH   1U
#define APP_FLASH_SECTOR_SIZE    (128UL * 1024UL)
#define APP_IO_BUFFER_SIZE       512U
#define APP_ENGINE_API           __attribute__((section(".app_engine_api")))

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint16_t abi_version;
    uint16_t target;
    uint32_t load_address;
    uint32_t payload_offset;
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint32_t icon_offset;
    uint32_t icon_size;
    uint16_t icon_width;
    uint16_t icon_height;
    uint16_t icon_format;
    uint16_t flags;
    char name[APP_ENGINE_NAME_MAX];
    char version[APP_ENGINE_VERSION_MAX];
    uint32_t header_crc32;
    uint8_t reserved[144];
} AppImageHeader;

_Static_assert(sizeof(AppImageHeader) == APP_IMAGE_HEADER_SIZE,
               "AppImageHeader size");
_Static_assert(APP_IMAGE_PAYLOAD_OFFSET == 0x1300U, "payload offset");

static uint8_t io_buffer[APP_IO_BUFFER_SIZE];

static void report_progress(AppEngineProgressCallback callback, void *context,
                            AppEngineProgressStage stage, uint32_t completed,
                            uint32_t total)
{
    if (callback != NULL) callback(stage, completed, total, context);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size)
{
    static const uint32_t table[16] = {
        0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL,
        0x76DC4190UL, 0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL,
        0xEDB88320UL, 0xF00F9344UL, 0xD6D6A3E8UL, 0xCB61B38CUL,
        0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL, 0xBDBDF21CUL
    };

    while (size-- != 0U) {
        crc ^= *data++;
        crc = (crc >> 4U) ^ table[crc & 0x0FUL];
        crc = (crc >> 4U) ^ table[crc & 0x0FUL];
    }
    return crc;
}

static uint32_t crc32_memory(const void *data, size_t size)
{
    return crc32_update(0xFFFFFFFFUL, data, size) ^ 0xFFFFFFFFUL;
}

static bool valid_app_id(const char *app_id)
{
    size_t length;
    if (app_id == NULL) return false;
    length = strlen(app_id);
    return (length != 0U) && (length <= APP_ENGINE_ID_MAX) &&
           (strstr(app_id, "..") == NULL) && (strchr(app_id, '/') == NULL) &&
           (strchr(app_id, '\\') == NULL) && (strchr(app_id, ':') == NULL);
}

static AppEngineStatus make_path(char *path, size_t size, const char *app_id)
{
    if (!valid_app_id(app_id)) return APP_ENGINE_INVALID_ARGUMENT;
    int length = snprintf(path, size, "%s/apps/%s/app.bin", SDPath, app_id);
    return ((length >= 0) && ((size_t)length < size))
               ? APP_ENGINE_OK : APP_ENGINE_INVALID_ARGUMENT;
}

static AppEngineStatus read_exact(FIL *file, void *data, UINT size)
{
    UINT read = 0U;
    FRESULT result = f_read(file, data, size, &read);
    if (result != FR_OK) return APP_ENGINE_IO_ERROR;
    return (read == size) ? APP_ENGINE_OK : APP_ENGINE_BAD_FORMAT;
}

static bool header_crc_valid(const AppImageHeader *header)
{
    AppImageHeader copy = *header;
    uint32_t expected = copy.header_crc32;
    copy.header_crc32 = 0U;
    return crc32_memory(&copy, sizeof(copy)) == expected;
}

static AppEngineStatus validate_header(const AppImageHeader *h, FSIZE_t size)
{
    uint64_t end = (uint64_t)h->payload_offset + h->payload_size;
    if ((h->magic != APP_IMAGE_MAGIC) ||
        (h->format_version != APP_ENGINE_FORMAT_VERSION) ||
        (h->header_size != APP_IMAGE_HEADER_SIZE) || !header_crc_valid(h))
        return APP_ENGINE_BAD_FORMAT;
    if ((h->abi_version != APP_ENGINE_ABI_VERSION) ||
        (h->target != APP_IMAGE_TARGET_FLASH) ||
        (h->load_address != APP_ENGINE_FLASH_BASE))
        return APP_ENGINE_INCOMPATIBLE;
    if ((h->icon_offset != APP_IMAGE_ICON_OFFSET) ||
        (h->icon_size != APP_ENGINE_ICON_SIZE) ||
        (h->icon_width != APP_ENGINE_ICON_WIDTH) ||
        (h->icon_height != APP_ENGINE_ICON_HEIGHT) ||
        (h->icon_format != APP_ENGINE_ICON_RGB565_LE) ||
        (h->payload_offset != APP_IMAGE_PAYLOAD_OFFSET))
        return APP_ENGINE_BAD_FORMAT;
    if ((h->payload_size < 8U) || (h->payload_size > APP_ENGINE_FLASH_SIZE))
        return APP_ENGINE_TOO_LARGE;
    return (end == size) ? APP_ENGINE_OK : APP_ENGINE_BAD_FORMAT;
}

static AppEngineStatus read_header(FIL *file, AppImageHeader *header)
{
    AppEngineStatus status = read_exact(file, header, sizeof(*header));
    return (status == APP_ENGINE_OK) ? validate_header(header, f_size(file))
                                     : status;
}

static void copy_text(char *dest, size_t dest_size, const char *src,
                      size_t src_size)
{
    size_t length = 0U;
    while ((length < src_size) && (src[length] != '\0')) ++length;
    if (length >= dest_size) length = dest_size - 1U;
    memcpy(dest, src, length);
    dest[length] = '\0';
}

static bool vector_valid(uint32_t sp, uint32_t reset, uint32_t payload_size)
{
    uint32_t pc = reset & ~1UL;
    return (sp >= 0x20000000UL) && (sp <= 0x20020000UL) &&
           ((sp & 7UL) == 0U) && ((reset & 1UL) != 0U) &&
           (pc >= APP_ENGINE_FLASH_BASE) &&
           (pc < APP_ENGINE_FLASH_BASE + payload_size);
}

static AppEngineStatus source_vector_valid(FIL *file,
                                           const AppImageHeader *header)
{
    uint32_t vector[2];
    if (f_lseek(file, header->payload_offset) != FR_OK)
        return APP_ENGINE_IO_ERROR;
    AppEngineStatus status = read_exact(file, vector, sizeof(vector));
    if (status != APP_ENGINE_OK) return status;
    return vector_valid(vector[0], vector[1], header->payload_size)
               ? APP_ENGINE_OK : APP_ENGINE_BAD_VECTOR;
}

static AppEngineStatus file_crc(FIL *file, uint32_t offset, uint32_t size,
                                uint32_t *result,
                                AppEngineProgressCallback callback,
                                void *context)
{
    uint32_t remaining = size;
    uint32_t crc = 0xFFFFFFFFUL;
    if ((result == NULL) || (f_lseek(file, offset) != FR_OK))
        return APP_ENGINE_IO_ERROR;
    while (remaining != 0U) {
        UINT count = remaining > sizeof(io_buffer) ? sizeof(io_buffer)
                                                   : (UINT)remaining;
        UINT read = 0U;
        if ((f_read(file, io_buffer, count, &read) != FR_OK) || (read != count))
            return APP_ENGINE_IO_ERROR;
        crc = crc32_update(crc, io_buffer, read);
        remaining -= read;
        report_progress(callback, context, APP_ENGINE_PROGRESS_VALIDATING,
                        size - remaining, size);
    }
    *result = crc ^ 0xFFFFFFFFUL;
    return APP_ENGINE_OK;
}

APP_ENGINE_API bool AppEngine_IsInstalled(const AppEngineInfo *info)
{
    if ((info == NULL) || (info->payload_size < 8U) ||
        (info->payload_size > APP_ENGINE_FLASH_SIZE)) return false;
    const uint32_t *vectors = (const uint32_t *)APP_ENGINE_FLASH_BASE;
    return vector_valid(vectors[0], vectors[1], info->payload_size) &&
           (crc32_memory(vectors, info->payload_size) == info->payload_crc32);
}

static void fill_info(const char *id, const AppImageHeader *header,
                      AppEngineInfo *info, bool check_installed)
{
    memset(info, 0, sizeof(*info));
    copy_text(info->app_id, sizeof(info->app_id), id, strlen(id));
    copy_text(info->name, sizeof(info->name), header->name, sizeof(header->name));
    copy_text(info->version, sizeof(info->version), header->version,
              sizeof(header->version));
    info->payload_size = header->payload_size;
    info->payload_crc32 = header->payload_crc32;
    info->icon_width = header->icon_width;
    info->icon_height = header->icon_height;
    info->icon_format = header->icon_format;
    info->installed = check_installed && AppEngine_IsInstalled(info);
}

static AppEngineStatus get_info(const char *id, AppEngineInfo *info,
                                bool check_installed)
{
    char path[64];
    FIL file;
    AppImageHeader header;
    if (info == NULL) return APP_ENGINE_INVALID_ARGUMENT;
    AppEngineStatus status = make_path(path, sizeof(path), id);
    if (status != APP_ENGINE_OK) return status;
    FRESULT result = f_open(&file, path, FA_READ);
    if (result != FR_OK)
        return ((result == FR_NO_FILE) || (result == FR_NO_PATH))
                   ? APP_ENGINE_NOT_FOUND : APP_ENGINE_IO_ERROR;
    status = read_header(&file, &header);
    if (status == APP_ENGINE_OK)
        fill_info(id, &header, info, check_installed);
    (void)f_close(&file);
    return status;
}

APP_ENGINE_API AppEngineStatus AppEngine_GetInfo(const char *id,
                                                 AppEngineInfo *info)
{
    return get_info(id, info, true);
}

APP_ENGINE_API AppEngineStatus AppEngine_ReadIcon(const char *id,
                                                  uint16_t *rgb565,
                                                  size_t pixels)
{
    char path[64];
    FIL file;
    AppImageHeader header;
    if ((rgb565 == NULL) || (pixels < APP_ENGINE_ICON_PIXELS))
        return APP_ENGINE_INVALID_ARGUMENT;
    AppEngineStatus status = make_path(path, sizeof(path), id);
    if (status != APP_ENGINE_OK) return status;
    if (f_open(&file, path, FA_READ) != FR_OK) return APP_ENGINE_NOT_FOUND;
    status = read_header(&file, &header);
    if ((status == APP_ENGINE_OK) &&
        (f_lseek(&file, header.icon_offset) != FR_OK)) status = APP_ENGINE_IO_ERROR;
    if (status == APP_ENGINE_OK)
        status = read_exact(&file, rgb565, APP_ENGINE_ICON_SIZE);
    (void)f_close(&file);
    return status;
}

APP_ENGINE_API AppEngineStatus AppEngine_Scan(AppEngineVisitor visitor,
                                              void *context,
                                              size_t *valid_count)
{
    char path[16];
    DIR directory;
    FILINFO entry;
    size_t count = 0U;
    int length = snprintf(path, sizeof(path), "%s/apps", SDPath);
    if ((length < 0) || ((size_t)length >= sizeof(path)))
        return APP_ENGINE_IO_ERROR;
    FRESULT result = f_opendir(&directory, path);
    if (result != FR_OK)
        return (result == FR_NO_PATH) ? APP_ENGINE_NOT_FOUND : APP_ENGINE_IO_ERROR;
    for (;;) {
        result = f_readdir(&directory, &entry);
        if (result != FR_OK) {
            (void)f_closedir(&directory);
            return APP_ENGINE_IO_ERROR;
        }
        if (entry.fname[0] == '\0') break;
        if (((entry.fattrib & AM_DIR) != 0U) && (entry.fname[0] != '.') &&
            valid_app_id(entry.fname)) {
            AppEngineInfo info;
            if (get_info(entry.fname, &info, false) == APP_ENGINE_OK) {
                ++count;
                if ((visitor != NULL) && !visitor(&info, context)) break;
            }
        }
    }
    (void)f_closedir(&directory);
    if (valid_count != NULL) *valid_count = count;
    return APP_ENGINE_OK;
}

static AppEngineStatus erase_slot(uint32_t payload_size,
                                  AppEngineProgressCallback callback,
                                  void *context)
{
    uint32_t sector_count = (payload_size + APP_FLASH_SECTOR_SIZE - 1U) /
                            APP_FLASH_SECTOR_SIZE;

    report_progress(callback, context, APP_ENGINE_PROGRESS_ERASING,
                    0U, sector_count);
    for (uint32_t index = 0U; index < sector_count; ++index) {
        FLASH_EraseInitTypeDef erase = {0};
        uint32_t sector_error;
        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        erase.Sector = FLASH_SECTOR_5 + index;
        erase.NbSectors = 1U;
        if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
            return APP_ENGINE_FLASH_ERROR;
        report_progress(callback, context, APP_ENGINE_PROGRESS_ERASING,
                        index + 1U, sector_count);
    }
    return APP_ENGINE_OK;
}

static AppEngineStatus program_payload(FIL *file, const AppImageHeader *header,
                                       AppEngineProgressCallback callback,
                                       void *context)
{
    uint32_t address = APP_ENGINE_FLASH_BASE;
    uint32_t remaining = header->payload_size;
    if (f_lseek(file, header->payload_offset) != FR_OK)
        return APP_ENGINE_IO_ERROR;
    while (remaining != 0U) {
        UINT count = remaining > sizeof(io_buffer) ? sizeof(io_buffer)
                                                   : (UINT)remaining;
        UINT read = 0U;
        if ((f_read(file, io_buffer, count, &read) != FR_OK) || (read != count))
            return APP_ENGINE_IO_ERROR;
        for (UINT offset = 0U; offset < read; offset += 4U) {
            uint32_t word = 0xFFFFFFFFUL;
            UINT available = read - offset;
            memcpy(&word, &io_buffer[offset], available < 4U ? available : 4U);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word) != HAL_OK)
                return APP_ENGINE_FLASH_ERROR;
            address += 4U;
        }
        remaining -= read;
        report_progress(callback, context, APP_ENGINE_PROGRESS_PROGRAMMING,
                        header->payload_size - remaining,
                        header->payload_size);
    }
    return APP_ENGINE_OK;
}

APP_ENGINE_API AppEngineStatus AppEngine_InstallWithProgress(
    const char *id, AppEngineProgressCallback callback, void *context)
{
    char path[64];
    FIL file;
    AppImageHeader header;
    uint32_t source_crc = 0U;
    report_progress(callback, context, APP_ENGINE_PROGRESS_PREPARING, 0U, 1U);
    AppEngineStatus status = make_path(path, sizeof(path), id);
    if (status != APP_ENGINE_OK) return status;
    if (f_open(&file, path, FA_READ) != FR_OK) return APP_ENGINE_NOT_FOUND;
    status = read_header(&file, &header);
    if (status == APP_ENGINE_OK) status = source_vector_valid(&file, &header);
    if (status == APP_ENGINE_OK)
        status = file_crc(&file, header.payload_offset, header.payload_size,
                          &source_crc, callback, context);
    if ((status == APP_ENGINE_OK) && (source_crc != header.payload_crc32))
        status = APP_ENGINE_BAD_CRC;
    if ((status == APP_ENGINE_OK) &&
        (crc32_memory((const void *)APP_ENGINE_FLASH_BASE,
                      header.payload_size) == header.payload_crc32)) {
        (void)f_close(&file);
        report_progress(callback, context, APP_ENGINE_PROGRESS_COMPLETE, 1U, 1U);
        return APP_ENGINE_OK;
    }
    if ((status == APP_ENGINE_OK) && (HAL_FLASH_Unlock() != HAL_OK))
        status = APP_ENGINE_FLASH_ERROR;
    if (status == APP_ENGINE_OK) {
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                               FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                               FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
        status = erase_slot(header.payload_size, callback, context);
    }
    if (status == APP_ENGINE_OK) {
        report_progress(callback, context, APP_ENGINE_PROGRESS_PROGRAMMING,
                        0U, header.payload_size);
        status = program_payload(&file, &header, callback, context);
    }
    (void)HAL_FLASH_Lock();
    (void)f_close(&file);
    if (status == APP_ENGINE_OK) {
        report_progress(callback, context, APP_ENGINE_PROGRESS_VERIFYING, 0U, 1U);
        if (crc32_memory((const void *)APP_ENGINE_FLASH_BASE,
                         header.payload_size) != header.payload_crc32)
            status = APP_ENGINE_BAD_CRC;
        else
            report_progress(callback, context, APP_ENGINE_PROGRESS_COMPLETE,
                            1U, 1U);
    }
    return status;
}

APP_ENGINE_API AppEngineStatus AppEngine_Install(const char *id)
{
    return AppEngine_InstallWithProgress(id, NULL, NULL);
}

APP_ENGINE_API AppEngineStatus AppEngine_ValidateInstalled(void)
{
    const uint32_t *vectors = (const uint32_t *)APP_ENGINE_FLASH_BASE;
    return vector_valid(vectors[0], vectors[1], APP_ENGINE_FLASH_SIZE)
               ? APP_ENGINE_OK : APP_ENGINE_BAD_VECTOR;
}

static void jump_to_application(uint32_t stack_pointer, uint32_t reset_handler)
    __attribute__((naked, noreturn));

static void jump_to_application(uint32_t stack_pointer, uint32_t reset_handler)
{
    (void)stack_pointer;
    (void)reset_handler;
    __asm volatile(
        "msr msp, r0\n"
        "movs r2, #0\n"
        "msr control, r2\n"
        "isb\n"
        "cpsie i\n"
        "bx r1\n");
}

APP_ENGINE_API void AppEngine_LaunchInstalled(void)
{
    if (AppEngine_ValidateInstalled() != APP_ENGINE_OK) NVIC_SystemReset();
    uint32_t sp = *(const uint32_t *)APP_ENGINE_FLASH_BASE;
    uint32_t reset = *(const uint32_t *)(APP_ENGINE_FLASH_BASE + 4U);
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    uint32_t banks = (SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1U;
    for (uint32_t index = 0U; index < banks; ++index) {
        NVIC->ICER[index] = 0xFFFFFFFFUL;
        NVIC->ICPR[index] = 0xFFFFFFFFUL;
    }
    SCB->VTOR = APP_ENGINE_FLASH_BASE;
    __DSB();
    __ISB();
    jump_to_application(sp, reset);
}

APP_ENGINE_API const char *AppEngine_StatusString(AppEngineStatus status)
{
    static const char *const text[] = {
        "ok", "end", "invalid argument", "not found", "I/O error",
        "bad format", "incompatible image", "image too large", "bad CRC",
        "bad vector table", "flash error"
    };
    return (unsigned)status < sizeof(text) / sizeof(text[0])
               ? text[status] : "unknown error";
}
