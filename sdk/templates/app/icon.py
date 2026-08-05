#!/usr/bin/env python3
import struct
from pathlib import Path

pixels = []
for y in range(48):
    for x in range(48):
        r, g, b = 10 + y // 4, 28 + y, 55 + y * 2
        if 8 <= x <= 39 and 9 <= y <= 38:
            r, g, b = 20, 105, 155
        if 13 <= x <= 34 and 14 <= y <= 33:
            r, g, b = 18, 35, 55
        if (x == 23 or y == 23) and 17 <= x <= 30 and 17 <= y <= 30:
            r, g, b = 255, 205, 55
        pixels.append(struct.pack('<H', ((r & 248) << 8) |
                                  ((g & 252) << 3) | (b >> 3)))
Path('build/icon.rgb565').write_bytes(b''.join(pixels))
