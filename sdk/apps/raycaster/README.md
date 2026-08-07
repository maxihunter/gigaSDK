# Tiny Raycaster

Minimal DDA raycaster demo for STM32F407VE and the 320x240 ILI9341 display.

Controls:

- Up/down: move forward/backward
- Left/right: rotate
- B or Menu: return to BIOS

The map and procedural texture palettes live in Flash. Rendering uses two
2560-byte RGB565 DMA buffers instead of a full framebuffer. While DMA sends
four scanlines, the CPU composes the next four. The bottom 30 pixels form a
persistent status bar; only its small FPS value area is updated once per second.

Build and package with `make package`.
