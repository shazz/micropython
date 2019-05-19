import pyb
import framebuf
import image

fbuf = bytearray(160*128*2)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)

img = image.Image(fb)
img.loadjpeg("images/test3.jpg")
tft.show(fb)
