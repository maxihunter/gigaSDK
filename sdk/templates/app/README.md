# Application template

This is a ready-to-copy application project for STM32F407 and gstr-bios. It
initializes the display, SD card/FatFs, keyboard, I2S audio mixer, video helper
and battery-backed RTC.

## Start a new application

```sh
cp -a sdk/templates/app sdk/apps/my_app
cd sdk/apps/my_app
```

Change `TARGET`, `APP_ID`, `APP_NAME` and `APP_VERSION` at the top of the
Makefile, replace the sample screen and icon, then build:

```sh
make package
```

Copy `build/apps/<app_id>` to `/apps/<app_id>` on the SD card.

## Removing unused functionality

Initialization is split into `App_InitDisplay`, `App_InitStorage`,
`App_InitKeyboard`, `App_InitAudio`, `App_InitVideo` and `App_InitClock`.
Delete the corresponding call from `App_Init()` when a subsystem is not used,
and remove all calls to that subsystem from the main loop. Compiler flags
`-ffunction-sections -fdata-sections` and linker flag `--gc-sections` discard
code and static data that can no longer be reached.

The Makefile intentionally contains every standard dependency. Leaving an
unused source in the list is safe; removing its line reduces build time.

Notes:

- `Audio_MixerProcess()` must run frequently while mixer playback is active.
- Audio files and `.vid` files are read through FatFs, so storage must remain
  initialized for those features.
- `RTC_Clock_Init()` preserves an already configured time using the RTC backup
  domain and its backup register.
- Button B in the sample calls `NVIC_SystemReset()` to return through BIOS.
