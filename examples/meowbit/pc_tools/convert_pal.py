#
# Convert JASC palettes to C structure
# Must be 16 or 256 palettes
#

import sys, os

if len(sys.argv) != 3:
    print("Usage: python convert_pal.py <file.pal> <nb_colors>")
    exit(-1)

print("Loading", str(sys.argv[1]))
path = os.path.splitext(str(sys.argv[1]))[0]
filename = os.path.basename(path)
nb_col = int(sys.argv[2])

if nb_col not in (16, 256):
    print("Number of colors {} not supported, should be 16 or 256".format(nb_col))
    exit(-1)    

c_str = "const uint16_t {0}_palette{1}[] = ".format(filename, nb_col) + "{\n"

with open(path + ".pal", "r") as fin:
    #  headers
    id = fin.readline()
    version = fin.readline()
    size = fin.readline()

    if "JASC-PAL" not in id:
        print("Wrong file format {}".format(id))
        exit(-1)
    
    for i in range(0, nb_col):
        colors = fin.readline().rstrip("\n\r").split(" ")
        c_str = c_str + "\tCOL(0x{:02X}{:02X}{:02X}), // {}\n".format(int(colors[0]), int(colors[1]), int(colors[2]), i)
    c_str = c_str + "};\n"

with open(path + ".c", "w") as fout:
    fout.write(c_str)
