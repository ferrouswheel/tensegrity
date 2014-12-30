# y = y0 + sqrt(r2 - (x - x0 )2 ).
import math

arc_widths = [ 331.66248, 314.466, 303.68112 ]
arc_heights = [ 29.289322, 19.526215, 9.763107 ]
mid_point = [ x/2.0 for x in arc_widths ]
origins = [ (165.83, -454.8), (157.23, -623.2), (151.84, -1175) ]
radii = [ 484.09, 642.81, 1185.63 ]

def height(x, r, origin):
    return origin[1] + math.sqrt((r*r) - ((x-origin[0])*(x-origin[0])))

def heights(index):
    print "for arc width ", arc_widths[index]
    for i in xrange(0, int(arc_widths[index]), int(arc_widths[index]/8)):
        print (i, height(i, radii[index], origins[index]))

heights(0)
heights(1)
heights(2)
