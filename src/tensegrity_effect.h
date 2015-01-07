#include "lib/effect.h"
#include <math.h>

#define NUM_STRUTS 4

class TEffect : public Effect
{
public:

    virtual void nextColor();

    int spokeNumberForIndex(int index) const;
    virtual void beginFrame(const FrameInfo &f);

    int strutOffsets[NUM_STRUTS + 1];
    float angleDelta;
};

inline void TEffect::nextColor() {}

inline int TEffect::spokeNumberForIndex(int index) const {
    for (unsigned i = 0; i < NUM_STRUTS + 1; i++) {
        if (index <= strutOffsets[i]) {
            return i - 1;
        }
    }
    fprintf(stderr, "No spoke for index %d\n", index);
    return -1;
}

inline void TEffect::beginFrame(const FrameInfo &f) {
    /*
     * Scan pixel positions and work out where each strut begins/ends
     */

    // scan for where each strut is at
    float last_coord_z = 100.0f;
    int strut = 0;
    //fprintf(stderr, "begin TEffect frame\n");
    for (unsigned i = 0; i < f.pixels.size(); i++) {
        if (f.pixels[i].point[2] < last_coord_z) {
            strutOffsets[strut] = i;
            //fprintf(stderr, "offset is %d\n", strutOffsets[strut]);
            strut++;
        }
        last_coord_z = f.pixels[i].point[2];
    }
    strutOffsets[strut] = f.pixels.size();
    //fprintf(stderr, "offset is %d\n", strutOffsets[strut]);

    angleDelta = M_PI*2.0/(NUM_STRUTS);

    //# colorsys hsv to rgb requires hue as 0 to 1 instead of degrees
    //for i, palette in enumerate(hues):
        //palette[channel] = float(palette[channel]) / 360.0
    
}
