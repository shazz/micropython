import pyb
import framebuf
import time
fbuf = bytearray(80*128)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL16, 160, framebuf.PAL16_C64)

f = open("images/trsi.p16", "rb")
f.readinto(fbuf)
f.close()
tft.show(fb)


tft.set_window(0,20,160,128)

fb.fill(3)
fb.fill_rect(0,0,160,128,4)
fb.text('hello,world', 32, 30, 2)

t0 = time.ticks_us()
tft.show(fb)
t1 = time.ticks_us()
dt = time.ticks_diff(t1, t0)
print(dt/1000)

import pyb
import framebuf
import time
fbuf = bytearray(160*128)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL256, 160, framebuf.PAL256_884)

f = open("images/test.p256", "rb")
f.readinto(fbuf)
f.close()
tft.show(fb)


import pyb
import framebuf
import time
fbuf = bytearray(160*128*2)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)
fb.fill(0x447711)
fb.fill_rect(0,0,160,128,4)
fb.text('hello,world', 32, 30, 2)

t0 = time.ticks_us()
tft.show(fb)
t1 = time.ticks_us()
dt = time.ticks_diff(t1, t0)
print(dt/1000)

