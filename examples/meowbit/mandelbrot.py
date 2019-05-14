import pyb
import framebuf
import time
import math

@micropython.native
def hsv2rgb(h, s, v):

    h60 = h / 60.0
    h60f = math.floor(h60)
    hi = int(h60f) % 6
    f = h60 - h60f
    p = v * (1 - s)
    q = v * (1 - f * s)
    t = v * (1 - (1 - f) * s)
    r, g, b = 0, 0, 0
    if hi == 0: r, g, b = v, t, p
    elif hi == 1: r, g, b = q, v, p
    elif hi == 2: r, g, b = p, v, t
    elif hi == 3: r, g, b = p, q, v
    elif hi == 4: r, g, b = t, p, v
    elif hi == 5: r, g, b = v, p, q

    r, g, b = int(r * 255) & 0xFF, int(g * 255) & 0xFF, int(b * 255) & 0xFF
    return ((r << 16) | (g << 8) | (b)) & 0xFFFFFF

@micropython.native
def mandelbrot(c, iterations):
    z = 0
    n = 0
    absolut = abs
    while absolut(z) <= 2 and n < iterations:
        z = z*z + c
        n += 1
    return n

@micropython.native
def plot(fb):
    WIDTH       = 160 - 1
    HEIGHT      = 128 - 1
    RE_START    = -2
    RE_END      = 1
    IM_START    = -1
    IM_END      = 1
    MAX_ITER    = 40
    RE_DELTA = RE_END - RE_START
    IM_DELTA = IM_END - IM_START

    blit = tft.show
    gtime = time.ticks_us
    setpixel = fb.pixel
    rx = range(0, WIDTH)
    ry = range(0, HEIGHT)
    t0 = gtime()

    for x in rx:
        for y in ry:
            # Convert pixel coordinate to complex number
            c = complex(RE_START + (x / WIDTH) * (RE_DELTA),
                        IM_START + (y / HEIGHT) * IM_DELTA)
            # Compute the number of iterations
            m = mandelbrot(c, MAX_ITER)
            
            # The color depends on the number of iterations
            #color = 255 - int(m * 255 / MAX_ITER)
            
            # The color depends on the number of iterations
            hue = int(255 * m / MAX_ITER)
            saturation = 255.0
            value = 255.0 if m < MAX_ITER else 0.0       

            rgb = hsv2rgb(hue, saturation, value)
            
            # Plot the point
            setpixel(x, y, rgb)
    blit(fb)
    t1 = gtime()

    dt = time.ticks_diff(t1, t0)
    print("Mandelbrot {} iterations computed and display in {} ms".format(MAX_ITER, dt/1000))

fbuf = bytearray(160*128*2)
tft = pyb.SCREEN()
fb = framebuf.FrameBuffer(fbuf, 160, 128, framebuf.RGB565)
fb.fill(0)
tft.show(fb)

plot(fb)

