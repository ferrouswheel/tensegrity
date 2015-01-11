// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include <termios.h>
#include "lib/color.h"
#include "tensegrity_effect.h"
#include "lib/effect_runner.h"
#include "lib/noise.h"
 
#define NUM_LAYERS 4
#define NUM_HUES 5
Effect** effects;

int hues[NUM_HUES][NUM_LAYERS] = {
    { 0, 60, 120, 280 },
    { 210, 240, 250, 260 },
    { 110, 140, 150, 160 },
    { 0, 20, 350, 320 },
    { 150, 240, 250, 60 }
};

bool randomEffects = false;

int NUM_EFFECTS=6;
const char* effectIndex[] = {
    "falling",
    "processing",
    "python",
    "perlin",
    "fire",
    "reverse"
};

#define PIV2 (M_PI+M_PI)
#define C360 360.0000000000000000000

double difangrad(double x, double y)
{
    double arg;

    arg = fmod(y-x, PIV2);
    if (arg < 0 )  arg  = arg + PIV2;
    if (arg > M_PI) arg  = arg - PIV2;

    return (-arg);
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
        : TEffect(channel), hue(0), t(0.0f) {}

    int hue;
    float t;
    float M_2PI;
    float time_divider;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);
        t += f.timeDelta;

        M_2PI = 2*M_PI;
        time_divider = M_2PI;
            
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        //const float speed = 10.0;
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

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};

class FireEffect : public TEffect
{
public:
    FireEffect(int channel)
        : TEffect(channel), hue(0), t(0.0f) {}

    int hue;
    float t;
    float minz, maxz;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);
        t += f.timeDelta;
        minz = f.modelMin[2];
        maxz = f.modelMax[2];
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        const float speed = 10.0;
        //float distance = len(p.point);
        float mahHue = hues[hue][0] / 360.0f;
        float height = (p.point[2] - minz) / (maxz - minz);
        float v = fbm_noise2(height, t, 4);
        hsv2rgb(rgb, mahHue, v - height, 0.4 + (v * 0.6));
    }

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};

class PerlinEffect : public TEffect
{
public:
    PerlinEffect(int channel)
        : TEffect(channel), hue(0), t(0.0f) {}

    int hue;
    float t;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);
        t += f.timeDelta;
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        const float speed = 10.0;
        //float distance = len(p.point);
        float mahHue = hues[hue][channel] / 360.0f;
        float v = fbm_noise4(p.point[0], p.point[1], p.point[2], t, 2);
        hsv2rgb(rgb, mahHue, 0.7, 0.4 + v);
    }

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};

class PythonEffect : public TEffect
{
public:
    PythonEffect(int channel)
        : TEffect(channel), hue(0), t(0.0f) {}

    int hue;
    float t;

    virtual void beginFrame(const FrameInfo &f)
    {
        TEffect::beginFrame(f);
        t += f.timeDelta;
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        const float speed = 10.0;
        //float distance = len(p.point);
        float mahHue = hues[hue][channel] / 360.0f;
        float v = fmodf((t * speed + p.index), 50.0f) / 50.0f;
        hsv2rgb(rgb, mahHue, 0.4, v);
    }

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};

class ReverseEffect : public TEffect
{
public:
    ReverseEffect(int channel)
        : TEffect(channel), hue(0), t(0.0f), revPerSecond(0.0f), currentAngle(0.0f) {}

    int hue;
    float t;
    float revPerSecond;
    float currentAngle;

    virtual void beginFrame(const FrameInfo &f)
    {
        revPerSecond = 1.0f;
        TEffect::beginFrame(f);
        currentAngle -= (f.timeDelta * ( 2*M_PI * (revPerSecond + 1.0)));
        if (currentAngle < 0) {
            currentAngle += M_PI * 2.0f;
        }
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        int spoke = spokeNumberForIndex(p.index);

        float spokeAngle = (angleDelta * spoke) - M_PI;

        //float d = currentAngle - spokeAngle;
        float angleDiff = difangrad(currentAngle - M_PI, spokeAngle); //atan2(fast_sin(d), fast_cos(d));
        float v = 1.0 - (2*(fabs(angleDiff) / (M_PI)));
        //fprintf(stderr, "currentAngle %.2f spoke %d angleDiff %.2f v %.2f\n", currentAngle, spoke, angleDiff, v);
        float mahHue = hues[hue][channel] / 360.0f;
        hsv2rgb(rgb, mahHue, 0.4, v);
    }

    virtual void nextColor() {
        hue += 1;
        if (hue >= NUM_HUES) hue = 0;
    }
};


void argumentUsage()
{
    fprintf(stderr, "[-v] [-fps LIMIT] [-speed MULTIPLIER] [-server HOST[:port]]");
}

void usage(const char *name)
{
    fprintf(stderr, "usage: %s ", name);
    argumentUsage();
    fprintf(stderr, "\n");
}

bool parseArgument(int &i, int &argc, char **argv, std::vector<EffectRunner*> er)
{
    std::vector<EffectRunner*>::const_iterator it;

    if (!strcmp(argv[i], "-v")) {
        for (it = er.begin(); it != er.end(); ++it) { (*it)->setVerbose(true); }
        return true;
    }

    if (!strcmp(argv[i], "-random")) {
        randomEffects = true;
        return true;
    }

    if (!strcmp(argv[i], "-fps") && (i+1 < argc)) {
        float rate = atof(argv[++i]);
        if (rate <= 0) {
            fprintf(stderr, "Invalid frame rate\n");
            return false;
        }
        for (it = er.begin(); it != er.end(); ++it) { (*it)->setMaxFrameRate(rate); }
        return true;
    }

    if (!strcmp(argv[i], "-speed") && (i+1 < argc)) {
        float speed = atof(argv[++i]);
        if (speed <= 0) {
            fprintf(stderr, "Invalid speed\n");
            return false;
        }
        for (it = er.begin(); it != er.end(); ++it) { (*it)->setSpeed(speed); }
        return true;
    }

    if (!strcmp(argv[i], "-server") && (i+1 < argc)) {
        ++i;
        for (it = er.begin(); it != er.end(); ++it) {
            if (!(*it)->setServer(argv[i])) {
                fprintf(stderr, "Can't resolve server name %s\n", argv[i]);
                return false;
            }
        }
        return true;
    }

    return false;
}

bool parseArguments(int argc, char **argv, std::vector<EffectRunner*> er)
{
    for (int i = 1; i < argc; i++) {
        if (!parseArgument(i, argc, argv, er)) {
            usage(argv[0]);
            return false;
        }
    }

    return true;
}

#define NB_ENABLE 1
#define NB_DISABLE 0
int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void nonblock(int state)
{
    struct termios ttystate;
 
    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);
 
    if (state==NB_ENABLE)
    {
        //turn off canonical mode
        ttystate.c_lflag &= ~ICANON;
        //minimum of number input read.
        ttystate.c_cc[VMIN] = 1;
    }
    else if (state==NB_DISABLE)
    {
        //turn on canonical mode
        ttystate.c_lflag |= ICANON;
    }
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
 
}

Effect* createEffect(const char *effectName, int channel)
{
    if (strcmp(effectName, "falling") == 0) {
        return new MyEffect(channel);
    }
    else if (strcmp(effectName, "python") == 0) {
        return new PythonEffect(channel);
    }
    else if (strcmp(effectName, "processing") == 0) {
        return new ProcessingEffect(channel);
    }
    else if (strcmp(effectName, "perlin") == 0) {
        return new PerlinEffect(channel);
    }
    else if (strcmp(effectName, "fire") == 0) {
        return new FireEffect(channel);
    }
    else if (strcmp(effectName, "reverse") == 0) {
        return new ReverseEffect(channel);
    }
    return NULL;
}

void changeEffect(int channel, const char* effectName, std::vector<EffectRunner*> er) {

    Effect *oldEffect = effects[channel];

    fprintf(stderr, "\nchange effect to %s\n", effectName);
    effects[channel] = createEffect(effectName, channel);
    
    er[channel]->setEffect(effects[channel]);
    delete oldEffect;
}

void checkController(std::vector<EffectRunner*> er) {
    /*
     * This currently just listens for keypresses on the console.
     * Eventually it should read data from the raspberry pi gpio pins.
     *
     */
    char c;
    static int effect = 0;
    std::vector<EffectRunner*>::const_iterator it;

    int i=kbhit();
    if (i!=0)
    {
        c=fgetc(stdin);
        if (c == 's') {
            float speed = 0.0f;

            for (it = er.begin(); it != er.end(); ++it) {
                speed = (*it)->getSpeed();
                if (speed < 0.1) {
                    speed = 5.0;
                } else {
                    speed *= 0.8;
                }
                (*it)->setSpeed(speed);
            }
            fprintf(stderr, "\nspeed set to %.2f. \n", speed);
        } else if (c == 'c') {
            // Change colour palette
            for (it = er.begin(); it != er.end(); ++it) {
                TEffect* e = (TEffect*) (*it)->getEffect();
                e->nextColor();
            }
            fprintf(stderr, "\nchanged color palette\n");
        } else if (c == 'e') {
            // Change effect
            int j=0;
            effect++;
            int e_index = effect % NUM_EFFECTS;
            fprintf(stderr, "\ne_index is %d\n", e_index);
            
            for (it = er.begin(); it != er.end(); ++it, ++j) {
                changeEffect(j, effectIndex[e_index], er);
            }
        } else {
            fprintf(stderr, "\nunknown command %c. \n", c);
        }
    }
}

int main(int argc, char **argv)
{
    std::vector<EffectRunner*> effect_runners;
    const int layers = NUM_LAYERS;
    char buffer[1000];

    effects = new Effect* [layers];

    for (int i = 0; i < layers; i++) {
        Effect *e = new ReverseEffect(i);
        effects[i] = e;

        EffectRunner* r = new EffectRunner();
        r->setEffect(e);
        r->setMaxFrameRate(50);

        r->setChannel(i);
        snprintf(buffer, 1000, "../Processing/tensegrity/opc_layout_%d.json", i);

        r->setLayout(buffer);
        effect_runners.push_back(r);
    }
    parseArguments(argc, argv, effect_runners);

    nonblock(NB_ENABLE);
    while (true) {
        for (int i=0 ; i < layers; i++) {
            effect_runners[i]->doFrame();
        }
        checkController(effect_runners);
    }
    nonblock(NB_DISABLE);

    return 0;
}
