#!/usr/bin/env python3
import struct
from pathlib import Path
p=[]
for y in range(48):
    for x in range(48):
        r,g,b=(18,75,43) if y>19 else (25,80,135)
        if y>20 and abs(x-24)<6+(y-20)//3: r,g,b=(62,65,70)
        if y>28 and abs(x-24)<3: r,g,b=(245,215,35)
        if y>35 and abs(x-24)<10: r,g,b=(240,55,75)
        p.append(struct.pack('<H',((r&248)<<8)|((g&252)<<3)|(b>>3)))
Path('build/icon.rgb565').write_bytes(b''.join(p))
