#
# Warning! Changing frequencies will disconnect the USB link but the 
# tests will continue.
# Simply reset the Meowbit and reload MicroPython and get the results
# in results.csv on th flash or on the SD card
#

import pyb
import framebuf
import time

def average(lst): 
    return sum(lst) / len(lst) 

def compute_time(fb):
    gtime = time.ticks_us
    tft = pyb.SCREEN()
    blit = tft.show

    t0 = gtime()
    blit(fb)
    t1 = gtime()
    dt = time.ticks_diff(t1, t0)

    return dt

def run_tests(freqs, fb):
    print("Starting tests...")
    pyb.freq(freqs[0], freqs[1], freqs[2], freqs[3])

    dt = []
    for i in range(0,10):
        dt.append(compute_time(fb))

    avg_dt = average(dt)
    return avg_dt

# reserve the biggest buffer needed. To avoid gc...
fbuf = bytearray(160*128*2)
results_str = "type;freqs;average\n"

# Default frequencies. Refer to the STM32F4 User Manual for limits
#sysclk: frequency of the CPU				: 56000000 
#hclk: frequency of the AHB bus, core memory and DMA	: 56000000
#pclk1: frequency of the APB1 bus			: 14000000
#pclk2: frequency of the APB2 bus			: 28000000

freqs_test = [
    [56000000, 56000000, 14000000, 28000000],
    [84000000, 84000000, 42000000, 84000000]
]

print("Loading RGB565 image...")
fb565 = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565)
f = open("images/test.r565", "rb")
f.readinto(fbuf)
f.close()

for freqs in freqs_test:
    avg_dt = run_tests(freqs, fb565)/1000
    print("At freqs {}, average blit time: {}ms".format(freqs, avg_dt))
    results_str += "{};{};{}\n".format("rgb565", freqs, avg_dt)

print("Loading PAL256 image...")
fb256 = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL256, 160, framebuf.PAL256_884)
f = open("images/test.p256", "rb")
f.readinto(fbuf)
f.close()

for freqs in freqs_test:
    avg_dt = run_tests(freqs, fb256)/1000
    print("At freqs {}, average blit time: {}ms".format(freqs, avg_dt))
    results_str += "{};{};{}\n".format("pal256", freqs, avg_dt)

print("Loading PAL16 image...")
fb16 = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.PAL16, 160, framebuf.PAL16_C64)
f = open("images/test.p16", "rb")
f.readinto(fbuf)
f.close()

for freqs in freqs_test:
    avg_dt = run_tests(freqs, fb16)/1000
    print("At freqs {}, average blit time: {}ms".format(freqs, avg_dt))
    results_str += "{};{};{}\n".format("pal16", freqs, avg_dt)

print(results_str)

with open("results.csv", "w") as text_file:
    print("{}".format(results_str), file=text_file)

print("Done!!!")



