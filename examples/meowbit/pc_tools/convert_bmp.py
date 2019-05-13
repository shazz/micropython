#
# Convert BMP to p16 or p256
# Images must be 160x128 and 16 or 256 palette mode
#

import PIL
from PIL import Image
import sys, os, math
import hashlib
import pickle

# Create a function called "chunks" with two arguments, l and n:
def chunks(l, n):
    # For item i in a range that is a length of l,
    for i in range(0, len(l), n):
        # Create an index range for l of n items:
        yield l[i:i+n]

def load_palette(filename):  
    with open(filename, "r") as fin:
        #  headers
        id = fin.readline()
        version = fin.readline()
        size = fin.readline()

        if "JASC-PAL" not in id:
            print("Wrong file format {}".format(id))
            exit(-1)
        
        rgb3 = []
        for i in range(0, nb_col):
            colors = fin.readline().rstrip("\n\r").split(" ")
            rgb3.append([int(colors[0]), int(colors[1]), int(colors[2]) ])
        
    return rgb3, hashlib.md5(pickle.dumps(rgb3)).hexdigest()

def remap(buffer, pal_src, pal_dest):
    remap = []
    for src_rgb_color in pal_src:
        sel = 999999999
        for num, dst_rgb_color in enumerate(pal_dest):
            dist = math.sqrt((dst_rgb_color[0] - src_rgb_color[0])**2 + (dst_rgb_color[1]  - src_rgb_color[1])**2 + (dst_rgb_color[2] - src_rgb_color[2])**2 )
            if dist < sel:
                idx = num
                sel = dist
        remap.append(idx)

    print("reammping table computed")
    for i in range(0, 256):
        print(i, pal_src[i], 'is mapped to index', remap[i], 'which is', pal_dest[remap[i]])

    buffer2 = bytearray()
    for old_col in buffer:
        new_col = remap[old_col]
        print(new_col)
        buffer2.append(new_col)

    return buffer2

if len(sys.argv) != 3:
    print("Usage: python convert_bmp.py <file.bmp> <nb_colors>")
    exit(-1)

print("Loading", str(sys.argv[1]))
path = os.path.splitext(str(sys.argv[1]))[0]
filename = os.path.basename(path)
nb_col = int(sys.argv[2])

im = PIL.Image.open(path + ".bmp")

# check format
if im.format != "BMP" or im.mode != "P" or im.info['compression'] != 0:
    print("Not au uncompressed BMP file in palette mode:", im.format, im.mode, im.info['compression'])
    exit(-1)   

# check size
if im.width != 160 or im.height != 128:
    print("Image size is not 160x128:", im.size)

# check palette size
pal = im.getpalette()
if len(pal)/3 not in (16, 256):
    print("Palette size is not 16 or 256", len(pal)/3)


# check palette is known
pal884, pal884_md5 = load_palette("../palettes/rgb884.pal")
pal686, pal686_md5 = load_palette("../palettes/rgb685.pal")
pal676, pal676_md5 = load_palette("../palettes/rgb676.pal")

buffer = bytearray()
for y in range(0, 128):
    for x in range(0, 160):
        pix = (im.getpixel( (x, y) )) & 0xFF
        buffer.append(pix)

# def remap_palette(self, dest_map, source_palette=None):
#im.show()
pal_rgb = list(chunks(pal, 3))
pal_rgb_md5 = hashlib.md5(pickle.dumps(pal_rgb)).hexdigest()

if pal_rgb_md5 not in(pal884_md5, pal686_md5, pal676_md5):
    print("not a known palette, trying to remap...")
    buffer = remap(buffer, pal_rgb, pal884)

print("buffer size:", len(buffer), '(should be', 160*128,'bytes)')

with open(path + ".p256", "wb") as fout:
    fout.write(buffer)
    print(filename + ".p256", 'saved')
