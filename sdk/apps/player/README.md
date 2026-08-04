# Giga Player

Place headerless signed 16-bit stereo PCM (`.pcm`) or GIMA v1 IMA ADPCM
(`.gim`) files beside `app.bin` in `/apps/player`. With the current FatFs
configuration filenames must fit 8.3.

- Up/Down: select track
- A: play/stop
- Left/Right: volume
- B: stop and reboot to BIOS

Build the installable image with `make package`.
