# Giga Video

Copy GVID v1 `.vid` files beside `app.bin` in `/apps/video`. Filenames must fit
8.3 with the current FatFs configuration.

- Up/Down: select video
- A: play
- Any key during playback: stop
- B: reboot to BIOS

Use `sdk/tools/video-prepare.sh` to convert source media and `make package` to
build the installable application image.
