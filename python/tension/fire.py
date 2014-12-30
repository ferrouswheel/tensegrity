import math
import colorsys

from tension import color_utils

strut_offsets = {}

angle_delta = None
hues = [
    {
        0: 0,
        1: 60,
        2: 120,
        3: 280,
    },
    {
        0: 210,
        1: 240,
        2: 250,
        3: 260
    },

]
current = 0


def init(channel, coords):
    # scan for where each strut is at
    global angle_delta

    strut_offsets[channel] = []

    last_coord_z = 0 
    for i, coord in enumerate(coords):
        if coord[2] < last_coord_z:
            strut_offsets[channel].append(i)
            print "offset is ", str(strut_offsets[channel][-1])
        last_coord_z = coord[2]
    strut_offsets[channel].append(len(coords))
    print "offset is ", str(strut_offsets[channel][-1])
    angle_delta = math.pi*2/len(strut_offsets[channel])

    # colorsys hsv to rgb requires hue as 0 to 1 instead of degrees
    for i, palette in enumerate(hues):
        palette[channel] = float(palette[channel]) / 360.0


def spoke_and_height(channel, index):
    for spoke, offset in enumerate(strut_offsets[channel]):
        if index <= offset:
            if spoke > 0:
                last_offset = strut_offsets[channel][spoke - 1]
            else:
                last_offset = 0
            height = float(index - last_offset) / float(offset - last_offset)
            return spoke, height
    print "No spoke for channel %d index %d" % (channel, index)
    return None, None


def pixel_color(t, channel, coord, index, total_pixels, random_seed):
    global current

    spoke, height = spoke_and_height(channel, index)

    if int((t / 10 % len(hues))) != current:
        print "change colors"
        current = int((t / 10 % len(hues)))
    
    #return (random_seed[(spoke*3)+0]*256, random_seed[(spoke*3)+2]*256, random_seed[(spoke*3)+1]*256)

    #template = [44, 44, 44]
    #if spoke < 3:
        #template[spoke] = 256
    #return tuple(template)

    #reverse = False
    #if channel % 2:
        #reverse = True
      
    #PI = math.pi
        
    #color(
         #((millis()/1000*2*PI) * ai % (2*PI))/ (2*PI) * 100,
         #float(channel)/numDepths * 80,
         #(k * millis()/500.0) % 255);
    speed = 100
    rgb = colorsys.hsv_to_rgb(
         hues[current][channel],
         0.5,
         color_utils.clamp(((t * speed + index) % 50.0) / 50.0)
         #0.8
         #color_utils.clamp((float(channel) / 4.0) * 80 / 100),
         #color_utils.clamp(spoke / 4.0)
         )
    pixel = tuple([x*255.0 for x in rgb])
    return pixel

