import pyb
import framebuf
import image

def showcb():
    tft.show(fb)  

fbuf = bytearray(160*128*2)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)

img = image.Image(fb)

#img.loadgif("images/small.gif", showcb)
img.loadgif("images/test256.gif", None)

