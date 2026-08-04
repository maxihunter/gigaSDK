#!/usr/bin/env python3
import struct
from pathlib import Path
out = []
for y in range(48):
    for x in range(48):
        r, g, b = 8 + y // 4, 16 + y // 3, 30 + y
        if 7 <= x <= 40 and 9 <= y <= 38:
            r, g, b = 24, 80, 126
        if 10 <= x <= 37 and 12 <= y <= 35:
            r, g, b = 7, 20, 35
        dx, dy = x - 24, y - 24
        if 18 <= dx + dy <= 29 and -12 <= dy <= 12 and dx >= -3:
            r, g, b = 255, 170, 65
        if (x in (8, 39)) and 12 <= y <= 35 and y % 6 < 3:
            r, g, b = 255, 205, 82
        out.append(struct.pack('<H', ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3)))
Path('build/icon.rgb565').write_bytes(b''.join(out))
