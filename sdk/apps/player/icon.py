#!/usr/bin/env python3
import struct
from pathlib import Path

pixels = []
for y in range(48):
    for x in range(48):
        r, g, b = 8 + y // 3, 18 + y // 2, 34 + y
        dx, dy = x - 24, y - 24
        if 9 * 9 < dx * dx + dy * dy < 19 * 19:
            r, g, b = 36, 150, 244
        if 20 <= x <= 25 and 11 <= y <= 29:
            r, g, b = 255, 180, 70
        if ((25 <= x <= 34) and (26 <= y <= 31) and (x + y >= 55)):
            r, g, b = 255, 180, 70
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        pixels.append(struct.pack("<H", value))
Path("build/icon.rgb565").write_bytes(b"".join(pixels))
