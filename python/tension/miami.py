from tension import color_utils

def pixel_color(t, coord, ii, n_pixels, random_values):
    """Compute the color of a given pixel.

    t: time in seconds since the program started.
    ii: which pixel this is, starting at 0
    coord: the (x, y, z) position of the pixel as a tuple
    n_pixels: the total number of pixels
    random_values: a list containing a constant random value for each pixel

    Returns an (r, g, b) tuple in the range 0-255

    """
    # make moving stripes for x, y, and z
    x, y, z = coord
    y += color_utils.cos(x + 0.2*z, offset=0, period=1, minn=0, maxx=0.6)
    z += color_utils.cos(x, offset=0, period=1, minn=0, maxx=0.3)
    x += color_utils.cos(y + z, offset=0, period=1.5, minn=0, maxx=0.2)

    # rotate
    x, y, z = y, z, x

#     # shift some of the pixels to a new xyz location
#     if ii % 17 == 0:
#         x += ((ii*123)%5) / n_pixels * 32.12 + 0.1
#         y += ((ii*137)%5) / n_pixels * 22.23 + 0.1
#         z += ((ii*147)%7) / n_pixels * 44.34 + 0.1

    # make x, y, z -> r, g, b sine waves
    r = color_utils.cos(x, offset=t / 4, period=2.5, minn=0, maxx=1)
    g = color_utils.cos(y, offset=t / 4, period=2.5, minn=0, maxx=1)
    b = color_utils.cos(z, offset=t / 4, period=2.5, minn=0, maxx=1)
    r, g, b = color_utils.contrast((r, g, b), 0.5, 1.4)

    clampdown = (r + g + b)/2
    clampdown = color_utils.remap(clampdown, 0.4, 0.5, 0, 1)
    clampdown = color_utils.clamp(clampdown, 0, 1)
    clampdown *= 0.9
    r *= clampdown
    g *= clampdown
    b *= clampdown

#     # shift the color of a few outliers
#     if random_values[ii] < 0.03:
#         r, g, b = b, g, r

    # black out regions
    r2 = color_utils.cos(x, offset=t / 10 + 12.345, period=4, minn=0, maxx=1)
    g2 = color_utils.cos(y, offset=t / 10 + 24.536, period=4, minn=0, maxx=1)
    b2 = color_utils.cos(z, offset=t / 10 + 34.675, period=4, minn=0, maxx=1)
    clampdown = (r2 + g2 + b2)/2
    clampdown = color_utils.remap(clampdown, 0.2, 0.3, 0, 1)
    clampdown = color_utils.clamp(clampdown, 0, 1)
    r *= clampdown
    g *= clampdown
    b *= clampdown

    # color scheme: fade towards blue-and-orange
#     g = (r+b) / 2
    g = g * 0.6 + ((r+b) / 2) * 0.4

#     # stretched vertical smears
#     v = color_utils.cos(ii / n_pixels, offset=t*0.1, period = 0.07, minn=0, maxx=1) ** 5 * 0.3
#     r += v
#     g += v
#     b += v

    # fade behind twinkle
    fade = color_utils.cos(t - ii/n_pixels, offset=0, period=7, minn=0, maxx=1) ** 20
    fade = 1 - fade*0.2
    r *= fade
    g *= fade
    b *= fade

    # twinkle occasional LEDs
    twinkle_speed = 0.07
    twinkle_density = 0.1
    twinkle = (random_values[ii]*7 + t*twinkle_speed) % 1
    twinkle = abs(twinkle*2 - 1)
    twinkle = color_utils.remap(twinkle, 0, 1, -1/twinkle_density, 1.1)
    twinkle = color_utils.clamp(twinkle, -0.5, 1.1)
    twinkle **= 5
    twinkle *= color_utils.cos(t - ii/n_pixels, offset=0, period=7, minn=0, maxx=1) ** 20
    twinkle = color_utils.clamp(twinkle, -0.3, 1)
    r += twinkle
    g += twinkle
    b += twinkle

    # apply gamma curve
    # only do this on live leds, not in the simulator
    #r, g, b = color_utils.gamma((r, g, b), 2.2)

    return (r*256, g*256, b*256)
