#!/usr/bin/env python

# vim: set ai et ts=4 sw=4:

from PIL import Image
import sys
import os

if len(sys.argv) < 2:
    print("Usage: {} <image-file>".format(sys.argv[0]))
    sys.exit(1)

fname = sys.argv[1]

img = Image.open(fname)

print("// h = %s, w = %s " % (img.height, img.width));
print("const uint8_t test_img[] = {");

#new_img = img.convert("RGB565");

for y in range(0, img.height):
    s = ""
    for x in range(0, img.width):
        (r, g, b) = img.getpixel( (x, y) )
        color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
        #color565 = ((color565 & 0xFF00) >> 8) | ((color565 & 0xFF) << 8)
        s += "0x{:02X}, ".format((color565 & 0xFF00) >> 8)
        s += "0x{:02X}, ".format((color565 & 0xFF))
    print(s)

print("};")
