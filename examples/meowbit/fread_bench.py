#
# Warning! Changing frequencies will disconnect the USB link but the 
# tests will continue.
# Simply reset the Meowbit and reload MicroPython and get the results
# in results.csv on th flash or on the SD card
#

import pyb
from pyb import LED 
import framebuf
import time
import gc

@micropython.native
def run_tests_read(filename, blk_size):
    dt = []
    gtime = time.ticks_us

    with open(filename, "rb") as f:
        for i in range(0,20):
            t0 = gtime()
            buf = f.read(blk_size)
            t1 = gtime()
            dt.append(time.ticks_diff(t1, t0))
            buf = None
            gc.collect()

    avg_dt = (sum(dt) / len(dt))/1000
    return avg_dt, blk_size

@micropython.native
def run_tests_readinto(filename, blk_size):
    buf = bytearray(blk_size)
    dt = []
    gtime = time.ticks_us
    bytes_read = 0

    with open(filename, "rb") as f:
        for i in range(0,20):
            t0 = gtime()
            bytes_read = f.readinto(buf)
            t1 = gtime()
            dt.append(time.ticks_diff(t1, t0))

    buf = None
    gc.collect()

    avg_dt = (sum(dt) / len(dt))/1000
    return avg_dt, bytes_read

for led in [LED(i) for i in range(1, 3)] : 
    led.on()

results_str = "function;block size;CPU;AHB;APB1;APB2;average time in ms\n"

# Default frequencies. Refer to the STM32F4 User Manual for limits
#sysclk: frequency of the CPU				: 56000000 
#hclk: frequency of the AHB bus, core memory and DMA	: 56000000
#pclk1: frequency of the APB1 bus			: 14000000
#pclk2: frequency of the APB2 bus			: 28000000

freqs_test = [
    [56000000, 56000000, 14000000, 28000000],
    [84000000, 84000000, 42000000, 84000000]
]
blk_sizes = [10, 100, 1000, 10000, 160*64, 160*128, 160*128*2]
gc.enable()
print("Free Mem: {} at start".format(gc.mem_free()))

filename = "unicode12.bin"

for blk in blk_sizes:

    for freqs in freqs_test:
        pyb.freq(freqs[0], freqs[1], freqs[2], freqs[3])
        avg_dt, bytes_read = run_tests_readinto(filename, blk)
        print("At freqs {}, average -readinto- time of {} bytes: {}ms".format(freqs, bytes_read, avg_dt))
        fr_cols = ';'.join(str(f) for f in freqs)
        results_str += "{};{};{};{}\n".format("readinto", bytes_read, fr_cols, avg_dt)

    for freqs in freqs_test:
        pyb.freq(freqs[0], freqs[1], freqs[2], freqs[3])
        avg_dt, bytes_read = run_tests_read(filename, blk)
        print("At freqs {}, average -read- time of {} bytes: {}ms".format(freqs, bytes_read, avg_dt))
        fr_cols = ';'.join(str(f) for f in freqs)
        results_str += "{};{};{};{}\n".format("read", bytes_read, fr_cols, avg_dt)

print(results_str)

with open("fileread.csv", "w") as text_file:
    print("{}".format(results_str), file=text_file)

print("Done!!!")
print("Free Mem: {} at end".format(gc.mem_free()))

for led in [LED(i) for i in range(1, 3)] : 
    led.off()


