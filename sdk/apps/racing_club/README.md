# Racing Club

Mode-7 style racing demo for STM32F407VE and ILI9341. The 32x32 track map is
stored as a const two-dimensional array in Flash. Rendering uses two 2560-byte
SPI DMA buffers.

- Up/down: accelerate/brake
- Left/right: steer
- B or Menu: return to BIOS

Build and package with `make package`.
