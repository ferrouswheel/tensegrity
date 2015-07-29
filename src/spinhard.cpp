// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include <termios.h>
#include <time.h>
#include <stdlib.h>

#include "lib/color.h"
#include "tensegrity_effect.h"
#include "rotaryencoder.h"
#include "lib/effect_runner.h"
 
#define ROTARY_TICKS_PER_REVOLUTION 360

Effect** effects;

bool randomEffects = false;
bool testSequence = false;

#define DEFAULT_FRAME_RATE 50

// 0 indicates that we should read the rpm from the rotary encoder
#if __APPLE__
int constant_rpm = 30;
#else
int constant_rpm = 0;
#endif

int NUM_EFFECTS=8;
const char* effectIndex[] = {
    "zootsuit",
    "plasma",
    "falling",
    "processing",
    "python",
    "perlin",
    "fire",
    "reverse"
};

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
    else if (strcmp(effectName, "plasma") == 0) {
        return new PlasmaEffect(channel);
    }
    else if (strcmp(effectName, "reverse") == 0) {
        return new ReverseEffect(channel);
    }
    else if (strcmp(effectName, "zootsuit") == 0) {
        return new ZootSuitEffect(channel);
    }
    else if (strcmp(effectName, "test") == 0) {
        return new TestSequenceEffect(channel);
    }
    return NULL;
}

bool changeSpeed(int channel, float speed, std::vector<EffectRunner*> er) {
    std::vector<EffectRunner*>::const_iterator it;
    if (speed <= 0.0f) {
        fprintf(stderr, "Invalid speed\n");
        return false;
    }
    if (channel >= ALL_LAYERS) {
        for (it = er.begin(); it != er.end(); ++it) { (*it)->setSpeed(speed); }
    } else {
        er[channel]->setSpeed(speed);
    }
    return true;
}

inline void _changeEffect(int channel, const char* effectName, std::vector<EffectRunner*> er) {
    Effect *oldEffect = effects[channel];

    effects[channel] = createEffect(effectName, channel);
    er[channel]->setEffect(effects[channel]);

    if (oldEffect != NULL) {
        if (effects[channel] != NULL) {
            ((TEffect*) effects[channel])->hue = ((TEffect*) oldEffect)->hue;
        }
        delete oldEffect;
    }
}

void changeEffect(int channel, const char* effectName, std::vector<EffectRunner*> &er) {
    fprintf(stderr, "\nchange effect to %s\n", effectName);
    if (channel >= ALL_LAYERS) {
        for (int i = 0; i < NUM_LAYERS; ++i) {
            _changeEffect(i, effectName, er);
        }
    } else {
        _changeEffect(channel, effectName, er);
    }
}

bool changeColour(int channel, std::vector<EffectRunner*> er) {
    std::vector<EffectRunner*>::const_iterator it;
    if (channel >= ALL_LAYERS) {
        for (it = er.begin(); it != er.end(); ++it) {
            TEffect* e = (TEffect*) (*it)->getEffect();
            e->nextColor();
        }
    } else {
        TEffect* e = (TEffect*) er[channel]->getEffect();
        e->nextColor();
    }
    return true;
}

bool parseArgument(int &i, int &argc, char **argv, std::vector<EffectRunner*> er)
{
    std::vector<EffectRunner*>::const_iterator it;

    if (!strcmp(argv[i], "-v")) {
        for (it = er.begin(); it != er.end(); ++it) { (*it)->setVerbose(true); }
        return true;
    }

    if (!strcmp(argv[i], "-test-sequence")) {
        testSequence = true;
        return true;
    }

    if (!strcmp(argv[i], "-random")) {
        randomEffects = true;
        return true;
    }

    if (!strcmp(argv[i], "-rpm")) {
        float rate = atof(argv[++i]);
        if (rate < 0) {
            fprintf(stderr, "Invalid rpm\n");
            return false;
        }
        constant_rpm = int(rate);
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
        return changeSpeed(ALL_LAYERS, speed, er);
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

void argumentUsage()
{
    fprintf(stderr, "[-v] [-fps LIMIT] [-random] [-speed MULTIPLIER] [-server HOST[:port]]");
}

void usage(const char *name)
{
    fprintf(stderr, "usage: %s ", name);
    argumentUsage();
    fprintf(stderr, "\n");
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
            float speed = er[0]->getSpeed();

            if (speed < 0.1) {
                speed = 5.0;
            } else {
                speed *= 0.8;
            }
            changeSpeed(ALL_LAYERS, speed, er);
            fprintf(stderr, "\nspeed set to %.2f. \n", speed);
        } else if (c == 'c') {
            // Change colour palette
            changeColour(ALL_LAYERS, er);
            fprintf(stderr, "\nchanged color palette\n");
        } else if (c == 'e') {
            // Change effect
            effect++;
            int e_index = effect % NUM_EFFECTS;
            changeEffect(ALL_LAYERS, effectIndex[e_index], er);
        } else {
            fprintf(stderr, "\nunknown command %c. \n", c);
        }
    }
}

void checkRandom(std::vector<EffectRunner*> er) {
    float chanceOfChange = 0.01f;

    float r = rand() / (float) RAND_MAX;

    if (r > chanceOfChange) {
        return;
    }

    fprintf(stderr, "\nrandom change... \n");

    // Select which channel we're affecting, with a bias towards all
    int channel = rand() % (NUM_LAYERS + 5);
    if (channel >= ALL_LAYERS) {
        channel = ALL_LAYERS;
        fprintf(stderr, "\nchange all channels... \n");
    } else {
        fprintf(stderr, "\nchange channel %d... \n", channel);
    }

    enum { EFFECT, COLOUR, SPEED };

    int changeType = rand() % 3;
    float speed = 0.0f;
    float maxSpeed = 6.0f;
    int ei;
    switch(changeType) {
        case EFFECT:
            ei = rand() % NUM_EFFECTS;
            fprintf(stderr, "\nchange effect to %s... \n", effectIndex[ei]);
            changeEffect(channel, effectIndex[ei], er);
            break;
        case COLOUR:
            fprintf(stderr, "\nchange colour... \n");
            changeColour(channel, er);
            break;
        case SPEED:
            speed = (maxSpeed * rand()) / (float) RAND_MAX;
            fprintf(stderr, "\nchange speed to %.2f... \n", speed);
            changeSpeed(channel, speed, er);
            break;
        default:
            break;
    }

}

float averageFrameRate(std::vector<EffectRunner*> er) {
    int effects = 0;
    float totalFrames = 0.0f;

    std::vector<EffectRunner*>::const_iterator it;

    for (it = er.begin(); it != er.end(); ++it) {
        totalFrames += (*it)->getFrameRate();
        effects += 1;
    }

    return totalFrames / effects;
}

float readAngle(struct encoder *encoder, float frameRate) {
    static float value = 0;
    static float ticks_per_second = ROTARY_TICKS_PER_REVOLUTION * ( constant_rpm / 60.0 );
    float l;

    if (encoder != NULL && constant_rpm > 0) {
        // shouldn't need this because interrupt is setup..
        //updateEncoders();
        l = encoder->value;
    } else {
        if (frameRate == 0) { return 0.0f; }
    
        l = value + (ticks_per_second / frameRate);
    }
    value = l;

    return PIV2 * (( int(value) % ROTARY_TICKS_PER_REVOLUTION ) / float(ROTARY_TICKS_PER_REVOLUTION));
}

void initEffects(std::vector<EffectRunner*> &effect_runners) {
    int j = 0;
    std::vector<EffectRunner*>::const_iterator it;
    for (it = effect_runners.begin(); it != effect_runners.end(); ++it, ++j) {
        if (testSequence) {
            changeEffect(j, "test", effect_runners);
        } else {
            changeEffect(j, effectIndex[0], effect_runners);
        }
    }
}

int main(int argc, char **argv)
{
    std::vector<EffectRunner*> effect_runners;
    const int layers = NUM_LAYERS;
    char buffer[1000];

    srand(time(NULL));

    effects = new Effect* [layers];

    for (int i = 0; i < layers; i++) {
        EffectRunner* r = new EffectRunner();
        effects[i] = NULL;
        r->setMaxFrameRate(DEFAULT_FRAME_RATE);

        r->setChannel(i);
        snprintf(buffer, 1000, "../Processing/tensegrity/opc_layout_%d.json", i);

        r->setLayout(buffer);
        effect_runners.push_back(r);
    }
    
    initEffects(effect_runners);

    parseArguments(argc, argv, effect_runners);
    if (testSequence) {
        initEffects(effect_runners);
    }

    struct encoder *encoder = NULL;
#if __APPLE__
#else
    if (constant_rpm) {
        wiringPiSetup();
        /*using pins 23/24*/
        encoder = setupencoder(4,5);
    }
#endif

    if (!randomEffects) {
        nonblock(NB_ENABLE);
    }
    while (true) {
        if (testSequence) {
            // TODO check testDone on each effect then disable testSequence
            // and replace effects 
            if (effects[0] != NULL) {
                if (((TestSequenceEffect*) effects[0])->testDone) {
                    testSequence = false;
                    initEffects(effect_runners);
                }
            } else {
                fprintf(stderr, "null pointer for test effect\n");
                return 1;
            }
        }
#if __APPLE__
        float fr = averageFrameRate(effect_runners);
        rotorAngle = readAngle(encoder, fr);
#else
        rotorAngle = readAngle(encoder, 0.0f);
#endif

        for (int i=0 ; i < layers; i++) {
            effect_runners[i]->doFrame();
        }
        if (randomEffects) {
            checkRandom(effect_runners);
        } else {
            checkController(effect_runners);
        }
    }
    if (!randomEffects) {
        nonblock(NB_DISABLE);
    }

    return 0;
}
