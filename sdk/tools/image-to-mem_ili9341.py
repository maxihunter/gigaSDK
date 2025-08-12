#!/usr/bin/env python

from PIL import Image
import sys
import os

if len(sys.argv) < 2:
    print("Usage: {} <image-file>".format(sys.argv[0]))
    sys.exit(1)

fname = sys.argv[1]

img = Image.open(fname)

print("#ifndef __%s__" % sys.argv[1].upper().replace(".", "_"));
print("#define __%s__" % sys.argv[1].upper().replace(".", "_"));
print("");
print("// h = %s, w = %s " % (img.height, img.width));
print("const PROGMEM uint8_t %s[] = {" % (sys.argv[1].split('.')[0]));

#new_img = img.convert("RGB565");
#new_name = sys.argv[0].split('.')[0] + "_rgb565.bmp"
#new_img.save(new_name, "BMP")

data = bytearray()

for y in range(0, img.height):
    s = ""
    for x in range(0, img.width):
        (r, g, b) = img.getpixel( (x, y) )
        color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
        #color565 = ((color565 & 0xFF00) >> 8) | ((color565 & 0xFF) << 8)
        s += "0x{:02X}, ".format((color565 & 0xFF00) >> 8)
        data.append((color565 & 0xFF00) >> 8)
        s += "0x{:02X}, ".format((color565 & 0xFF))
        data.append(color565 & 0xFF)
    print(s)

print("};")

file_path = fname+".565"
newFile = open(file_path, "wb")
# write to file
newFile.write(data)

print("#endif // __%s__" % sys.argv[1].upper().replace(".", "_"));
