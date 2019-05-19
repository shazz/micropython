import pyb
from pyb import LED 

import framebuf
import image

import time
import gc
import machine

@micropython.native
def display_jpeg(filename):
    fbuf = bytearray(160*128*2)
    tft = pyb.SCREEN()
    fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)
    img = image.Image(fb)
    gtime = time.ticks_us
    jpg = img.loadjpeg
    dt = []

    for i in range(0,20):
        t0 = gtime()
        jpg(filename)
        t1 = gtime()
        dt.append(time.ticks_diff(t1, t0))
        tft.show(fb)

    avg_dt = (sum(dt) / len(dt))/1000
    fbuf = None
    fb = None
    img = None
    gc.collect()
    return avg_dt

@micropython.native
def display_bmp(filename):
    fbuf = bytearray(160*128*2)
    tft = pyb.SCREEN()
    fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)
    img = image.Image(fb)
    gtime = time.ticks_us
    bmp = img.loadbmp
    dt = []

    for i in range(0,20):
        t0 = gtime()
        bmp(filename)
        t1 = gtime()
        dt.append(time.ticks_diff(t1, t0))
        tft.show(fb)

    avg_dt = (sum(dt) / len(dt))/1000
    fbuf = None
    fb = None
    img = None
    gc.collect()
    return avg_dt    

@micropython.native
def display_gif(filename):
    fbuf = bytearray(160*128*2)
    tft = pyb.SCREEN()
    fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565, 160)
    img = image.Image(fb)
    gtime = time.ticks_us
    gif = img.loadgif
    dt = []

    for i in range(0,20):
        t0 = gtime()
        gif(filename, None)
        t1 = gtime()
        dt.append(time.ticks_diff(t1, t0))
        tft.show(fb)

    avg_dt = (sum(dt) / len(dt))/1000
    fbuf = None
    fb = None
    img = None
    gc.collect()
    return avg_dt      

@micropython.native
def display_p16(filename):
    fbuf = bytearray(80*128)
    tft = pyb.SCREEN()
    fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL16, 160, framebuf.FRAMEBUF_PAL16_C64)
    img = image.Image(fb)
    gtime = time.ticks_us
    gif = img.loadp16
    dt = []

    for i in range(0,20):
        t0 = gtime()
        gif(filename)
        t1 = gtime()
        dt.append(time.ticks_diff(t1, t0))
        tft.show(fb)

    avg_dt = (sum(dt) / len(dt))/1000
    fbuf = None
    fb = None
    img = None
    gc.collect()
    return avg_dt    

@micropython.native
def display_p256(filename):
    fbuf = bytearray(160*128)
    tft = pyb.SCREEN()
    fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL256, 160, framebuf.PAL256_884)
    img = image.Image(fb)
    gtime = time.ticks_us
    p16 = img.loadp256
    dt = []

    for i in range(0,20):
        t0 = gtime()
        p16(filename)
        t1 = gtime()
        dt.append(time.ticks_diff(t1, t0))
        tft.show(fb)

    avg_dt = (sum(dt) / len(dt))/1000
    fbuf = None
    fb = None
    img = None
    gc.collect()
    return avg_dt              

def run_tests():

    results_str = "deocder;format;CPU;AHB;APB1;APB2;average time in ms\n"
    freqs_test = [
        [56000000, 56000000, 14000000, 28000000],
        [84000000, 84000000, 42000000, 84000000]
    ]

    # Switch on LEDs to show start
    for led in [LED(i) for i in range(1, 3)] : 
        led.on()

    for freqs in freqs_test:
        fr_cols = ';'.join(str(f) for f in freqs)
        pyb.freq(freqs[0], freqs[1], freqs[2], freqs[3])

        avg_dt_jpg = display_jpeg("images/test3.jpg")
        results_str += "{};{};{};{}\n".format("jpeg", "RGB565", fr_cols, avg_dt_jpg)

        avg_dt_bmp24 = display_bmp("images/test24.bmp")
        results_str += "{};{};{};{}\n".format("bmp", "RGB565", fr_cols, avg_dt_bmp24)

        #avg_dt_bmp256 = display_bmp("images/test256.bmp")
        #results_str += "{};{};{};{}\n".format("bmp", "PAL256", fr_cols, avg_dt_bmp256)      

        #avg_dt_bmp16 = display_bmp("images/test16.bmp")
        #results_str += "{};{};{};{}\n".format("bmp", "PAL16", fr_cols, avg_dt_bmp16)             

        #avg_dt_gif = display_gif("images/test256.gif")
        #results_str += "{};{};{};{}\n".format("gif", "RGB565", fr_cols, avg_dt_gif)        

        #avg_dt_p16 = display_p16("images/test.p16")
        #results_str += "{};{};{};{}\n".format("p16", "PAL16", fr_cols, avg_dt_p16)     

        #avg_dt_p256 = display_p256("images/test.p256")
        #results_str += "{};{};{};{}\n".format("p256", "PAL256", fr_cols, avg_dt_p256)             

    print(results_str)

    with open("fileread.csv", "w") as text_file:
        print("{}".format(results_str), file=text_file)        

    # Switch off LEDs to show end
    for led in [LED(i) for i in range(1, 3)] : 
        led.off()        

run_tests()
machine.reset()