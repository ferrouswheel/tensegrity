#!/usr/bin/env python
import math

radius = 1.0 # m
concentric_rings = 3.0 # m
rps = 2.0 # revs per second
arm_mass = 4.0 # kg

if __name__ == "__main__":
    diff = radius / (concentric_rings - 1)
    for i in range(0, int(concentric_rings)):
        print "ring %d" % (i,),
        diameter = (diff * i) * 2
        velocity = rps * diameter * math.pi
        print " d=%.2fm v=%.2fms-1 " % (diameter, velocity)
        if diameter > 0:
            print " f=%.2f " % ((arm_mass * velocity * velocity) / (diameter / 2.0), )


