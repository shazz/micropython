import pyb
import framebuf
import image

fbuf = bytearray(80*128)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL16, 160, framebuf.PAL16_C64)

with open("images/test.p16", "rb") as f:
    f.readinto(fbuf)
    tft.show(fb)

pyb.delay(2000)

img = image.Image(fb)
img.loadp16("images/dragoon.p16")
tft.show(fb)