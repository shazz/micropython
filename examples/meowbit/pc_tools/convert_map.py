#
# Convert Unrestricted C64 Bitmap pictures (*.map) to p16
#

import sys, os

if len(sys.argv) != 2:
    print("Usage: python convert_map.py <file.map>")
    exit(-1)

print("Loading", str(sys.argv[1]))
filename = os.path.splitext(str(sys.argv[1]))[0]

buffer = bytearray()
counter = 160*128

with open(filename + ".map", "rb") as fin:
    while counter > 0:
        word = fin.read(2)
        b1 = (word[0] & 0xf0)
        b2 = (word[1] & 0x0f)
        b3 = (b1 | b2)
        buffer.append(b3)
        counter = counter - 2

print("buffer size:", len(buffer), '(should be', 160*128>>1,'bytes)')
with open(filename + ".p16", "wb") as fout:
    fout.write(buffer)
    print(filename + ".p16", 'saved')

