#!/usr/bin/env python3
import struct
from pathlib import Path

pixels = []
for y in range(48):
    for x in range(48):
        sky = y < 24
        r, g, b = ((7, 18 + y, 36 + y) if sky else (16, 18, 25))
        edge = 8 + abs(24 - x) // 2
        if not sky and y < 43 and (x < edge or x > 47 - edge):
            r, g, b = (210, 55 + (y & 7) * 5, 85)
        if 21 <= x <= 26 and 15 <= y <= 41:
            r, g, b = (40, 205, 245)
        value = ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3)
        pixels.append(struct.pack('<H', value))
Path('build/icon.rgb565').write_bytes(b''.join(pixels))
