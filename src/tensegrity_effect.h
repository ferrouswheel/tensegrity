#include "lib/effect.h"
#include "lib/noise.h"
#include <math.h>

#define NUM_STRUTS 4

#define NUM_LAYERS 4
#define ALL_LAYERS NUM_LAYERS

#define NUM_HUES 7

#define PIV2 (M_PI+M_PI)
#define C360 360.0000000000000000000

double diffAngleRadians(double x, double y)
{
    double arg;

    arg = fmod(y-x, PIV2);
    if (arg < 0 )  arg  = arg + PIV2;
    if (arg > M_PI) arg  = arg - PIV2;

    return (-arg);
}

// should probably be passed through effect runner, but that requires more
// modification to the default fadecandy code.
float rotorAngle = 0.0f;

int hues[NUM_HUES][NUM_LAYERS] = {
    { 0, 60, 120, 280 },
    { 210, 240, 250, 260 },
    { 110, 140, 150, 160 },
    { 0, 20, 350, 320 },
    { 150, 240, 250, 60 },
    { 197, 227, 167, 197 },
    { 300, 330, 151, 120 }
};

class TEffect : public Effect
{
public:
    // Channels:
    // 0 is outer radius
    // 1 is reversed
    // 2 is inner radius
    // 3 is not used
    int channel;
    TEffect(int _channel) : channel(_channel), hue(0), t(0.0f) {};

    virtual void nextColor();

    int spokeNumberForIndex(int index) const;
    virtual void beginFrame(const FrameInfo &f);

    int strutOffsets[NUM_STRUTS + 1];
    float angleDelta;

    float spokeAngles[NUM_STRUTS];
    float spokeAngleOffsets[NUM_STRUTS];

    int hue;
    float t;
    float minz, maxz;
    float layerHeight;

    int spokeWrap(int spoke) const {
        // + NUM_STRUTS to ensure we're positive when given a -1
        return (spoke + NUM_STRUTS) % int(NUM_STRUTS);
    }

};

inline void TEffect::nextColor() {
    hue += 1;
    if (hue >= NUM_HUES) hue = 0;
}

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
    // Scan pixel positions and work out where each strut begins/ends
    float last_coord_z = 100.0f;
    int strut = 0;
    //fprintf(stderr, "begin TEffect frame\n");
    for (unsigned i = 0; i < f.pixels.size(); i++) {
        if (f.pixels[i].point[2] < last_coord_z) {
            strutOffsets[strut] = i;
            strut++;
        }
        last_coord_z = f.pixels[i].point[2];
    }
    strutOffsets[strut] = f.pixels.size();

    angleDelta = M_PI*2.0/(NUM_STRUTS);

    for (unsigned i = 0; i < NUM_STRUTS; i++) {
        spokeAngles[i] = angleDelta * i;
        spokeAngleOffsets[i] = diffAngleRadians(-rotorAngle, spokeAngles[i]);
    }

    t += f.timeDelta;
    minz = f.modelMin[2];
    maxz = f.modelMax[2];
    layerHeight = (maxz - minz);
}

class MyEffect : public TEffect
{
public:
    MyEffect(int channel)
        : TEffect(channel), cycle (0), hue(0.2){}

    virtual ~MyEffect() {}

    float cycle;
    float hue;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);
        const float speed = 10.0;
        cycle = fmodf(cycle + f.timeDelta * speed, 2 * M_PI);
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        float distance = len(p.point);
        float wave = sinf(3.0 * distance - cycle) + noise3(p.point);
        int spoke = spokeNumberForIndex(p.index);
        hsv2rgb(rgb, hue, 1.0 - (0.2 * (spoke+1)), wave);
    }

    virtual void nextColor() {
        hue += 0.1f;
        if (hue >= 1.0f) {
            hue = 0.0f;
        }

    }
};

class ProcessingEffect: public TEffect
{
public:
    ProcessingEffect(int channel)
        : TEffect(channel) {}

    float M_2PI;
    float time_divider;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);

        M_2PI = 2*M_PI;
        time_divider = M_2PI;
            
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        int spoke = spokeNumberForIndex(p.index);

        bool reverse = false;
        if (channel % 2 != 0) { reverse = true; }
      
        int i = spoke;
        float ai = i * angleDelta;

        float k = p.index - strutOffsets[spoke];
        float mahHue = hues[hue][channel];
        
        hsv2rgb(rgb,
           fmodf(mahHue, 360.0f) / 360.0f,
           (float(channel) / float(NUM_LAYERS)) * 0.50f,
           fmodf(k * t/0.5, 100) / 100.0f
        );
    }

};

class FireEffect : public TEffect
{
public:
    FireEffect(int channel)
        : TEffect(channel) {}

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        float mahHue = hues[hue][0] / 360.0f;
        float height = (p.point[2] - minz) / (maxz - minz);
        height += (maxz*channel);
        float v = fbm_noise2(height, t, 4);
        hsv2rgb(rgb, mahHue, v - height, 0.4 + (v * 0.6));
    }

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};

class PlasmaEffect : public TEffect
{
public:
    PlasmaEffect(int channel)
        : TEffect(channel) {}

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        float mahHue = hues[hue][channel] / 360.0f;
        float height = (p.point[2] - minz);
        float v = fbm_noise2(height, t, 4);
        float heightRange = layerHeight * NUM_LAYERS;
        float val = 0.5f + (2.0f * v * (height + (channel * layerHeight)));
        float sat = (0.2f * channel / 3.0f) + (0.9 * height / layerHeight);
        //fprintf(stderr,
           //"v %.2f height %.2f layerHeight %.2f val %.2f\n", v, height, layerHeight, val);
        hsv2rgb(rgb, mahHue, sat, val);
    }

};

class PerlinEffect : public TEffect
{
public:
    PerlinEffect(int channel)
        : TEffect(channel) {}


    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        const float speed = 10.0;
        float mahHue = hues[hue][channel] / 360.0f;
        float v = fbm_noise4(p.point[0], p.point[1], p.point[2], t, 2);
        hsv2rgb(rgb, mahHue, 0.7, 0.4 + v);
    }

};

class PythonEffect : public TEffect
{
public:
    PythonEffect(int channel)
        : TEffect(channel) {}

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        const float speed = 10.0;
        float mahHue = hues[hue][channel] / 360.0f;
        float v = fmodf((t * speed + p.index), 50.0f) / 50.0f;
        hsv2rgb(rgb, mahHue, 0.4, v);
    }

};

class ReverseEffect : public TEffect
{
public:
    ReverseEffect(int channel)
        : TEffect(channel) {}

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        int spoke = spokeNumberForIndex(p.index);
        float angleDiff = spokeAngleOffsets[spoke];

        float v = 1.0 - (2*(fabs(angleDiff) / (M_PI)));
        //fprintf(stderr,
        //   "currentAngle %.2f spoke %d angleDiff %.2f v %.2f\n", currentAngle, spoke, angleDiff, v);
        float mahHue = hues[hue][channel] / 360.0f;
        hsv2rgb(rgb, mahHue, 0.4, v);
    }

};

class ZootSuitEffect : public TEffect
{
public:
    ZootSuitEffect(int channel)
        : TEffect(channel) {}

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        int spoke = spokeNumberForIndex(p.index);
        float mahHue = hues[hue][channel] / 360.0f;

        float angleDiff = spokeAngleOffsets[spoke];
        float height = (p.point[2] - minz);

        float _t = t * 1.0f;
        int activeSpoke = spokeWrap(int((_t / layerHeight)));
        float activeHeight = fmod(_t, layerHeight);

        if (channel % 2 == 1) {
            // middle strut goes bottom to top and has to be shifted forward one
            // to cross over at the same time as the other channels.
            activeSpoke = spokeWrap(activeSpoke + 1);
            height = layerHeight - height;
        }

#ifdef DEBUG_ZOOT
        if (activeSpoke == spoke) {
            mahHue = 0.9;
        } else {
            mahHue = 0.7;
        }
#endif // DEBUG_ZOOT

        if ( spokeWrap(activeSpoke - 1) == spoke ||
             spokeWrap(activeSpoke + 1) == spoke ) {
#ifdef DEBUG_ZOOT
            mahHue = 0.1;
#endif // DEBUG_ZOOT
            if (height < layerHeight/2.0f) {
                height = layerHeight + height;
            } else {
                height = height - layerHeight;
            }
        }

#ifdef DEBUG_ZOOT
        float dist = fabs(activeHeight - height);
        if (dist > 0.4f) {
            hsv2rgb(rgb, mahHue, 0.8, 0.0f);
            return;
        }
        float v = 1.0f;

        fprintf(stderr,
           "spoke %d:%d active %d - point height %.2f / %.2f active %.2f  - v %.2f\n", channel, spoke, activeSpoke, height, layerHeight, activeHeight, v);
#else
        float v = 1.0f - fabs(activeHeight - height);
#endif // DEBUG_ZOOT

        hsv2rgb(rgb, mahHue, 0.8, v);
    }

};

class TestSequenceEffect : public TEffect
{
public:
    TestSequenceEffect(int channel)
        : TEffect(channel), loops(0), lastSpoke(0), testDone(false) {}

    int loops;
    int lastSpoke;
    bool testDone;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);

        float _t = t / 1.0f;
        int activeSpoke = spokeWrap(int((_t / layerHeight)));

        if (activeSpoke == 0 and activeSpoke != lastSpoke) {
            loops += 1;
            if (loops >= 5) {
                testDone = true;
                fprintf(stderr, "test sequence finished!\n");
            }
        }
        lastSpoke = activeSpoke;
    }


    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        int spoke = spokeNumberForIndex(p.index);
        float mahHue = 0.1;

        //float angleDiff = spokeAngleOffsets[spoke];
        //float height = (p.point[2] - minz);
        //bool upper = false;
        //if (height > layerHeight / 2.0) {
            //upper = true;
        //}

        float _t = t / 1.0f;
        int activeSpoke = spokeWrap(int((_t / layerHeight)));

        if (activeSpoke == spoke) {
            float splitSize = 0.0f;
            if (activeSpoke > 0) {
                splitSize = layerHeight / (1 + activeSpoke * 2);
            }
            if (splitSize > 0.0f) {
                float height = (p.point[2] - minz);

                if (int(height / splitSize) % 2 == 0) {
                    hsv2rgb(rgb, mahHue, 0.8, 0.8);
                } else {
                    hsv2rgb(rgb, mahHue, 0.8, 0.1);
                }
            } else {
                hsv2rgb(rgb, mahHue, 0.8, 0.8);
            }
        } else {
            hsv2rgb(rgb, mahHue, 0.8, 0.0);
        }
    }

};
