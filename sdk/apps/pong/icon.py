#!/usr/bin/env python3
import struct
from pathlib import Path
data=[]
for y in range(48):
    for x in range(48):
        r,g,b=5+y//5,12+y//3,28+y
        if 7<=x<=11 and 12<=y<=36: r,g,b=45,185,255
        if 36<=x<=40 and 12<=y<=36: r,g,b=255,70,170
        if 22<=x<=27 and 21<=y<=26: r,g,b=255,205,70
        if x in (23,24) and y%8<4: r,g,b=80,100,130
        data.append(struct.pack('<H',((r&248)<<8)|((g&252)<<3)|(b>>3)))
Path('build/icon.rgb565').write_bytes(b''.join(data))
