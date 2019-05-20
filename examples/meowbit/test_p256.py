import pyb
import framebuf
import image

fbuf = bytearray(160*128)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL256, 160, framebuf.PAL256_884)

with open("images/test.p256", "rb") as f:
    f.readinto(fbuf)
    tft.show(fb)

pyb.delay(2000)

img = image.Image(fb)
img.loadp256("images/test.p256")
tft.show(fb)