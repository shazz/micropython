import pyb
import framebuf
import image

fbuf = bytearray(160*128*2)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)

img = image.Image(fb)
img.loadbmp("images/test24.bmp")
tft.show(fb)
