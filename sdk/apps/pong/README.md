# Neon Pong

- Main menu: Up/Down, A to select, Left/Right to change AI difficulty or Music On/Off
- Match: Up/Down moves the paddle
- B pauses; A resumes or B quits from the pause screen
- First to seven points wins
- Menu navigation and ball collisions are played as non-blocking effects over the music
- Background music loops until it is disabled in the menu or the application exits

Run `make package` and copy the complete `build/apps/pong` directory to
`/apps/pong` on the SD card. The directory contains `app.bin`, `pong_bg.gim`
and the music license file.
