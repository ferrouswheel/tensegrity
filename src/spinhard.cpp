// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include <termios.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/effect_runner.h"
#include "lib/noise.h"

class MyEffect : public Effect
{
public:
    MyEffect()
        : cycle (0) {}

    float cycle;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float speed = 10.0;
        cycle = fmodf(cycle + f.timeDelta * speed, 2 * M_PI);
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        float distance = len(p.point);
        float wave = sinf(3.0 * distance - cycle) + noise3(p.point);
        hsv2rgb(rgb, 0.2, 0.3, wave);
    }
};

void argumentUsage()
{
    fprintf(stderr, "[-v] [-fps LIMIT] [-speed MULTIPLIER] [-layout FILE.json] [-server HOST[:port]]");
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

void checkController(std::vector<EffectRunner*> er) {
    char c;
    int i=kbhit();
    if (i!=0)
    {
        c=fgetc(stdin);
        if (c == 's') {
            std::vector<EffectRunner*>::const_iterator it;
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
        } else {
            fprintf(stderr, "\nunknown command %c. \n", c);
        }
    }
}

int main(int argc, char **argv)
{
    std::vector<EffectRunner*> effect_runners;
    const int layers = 4;
    char buffer[1000];
 
    nonblock(NB_ENABLE);

    MyEffect e;

    for (int i = 0; i < layers; i++) {
        EffectRunner* r = new EffectRunner();
        r->setEffect(&e);
        r->setMaxFrameRate(50);

        r->setChannel(i);
        snprintf(buffer, 1000, "../Processing/tensegrity/opc_layout_%d.json", i);

        r->setLayout(buffer);
        effect_runners.push_back(r);
    }
    parseArguments(argc, argv, effect_runners);

    while (true) {
        for (int i=0 ; i < layers; i++) {
            effect_runners[i]->doFrame();
        }
        checkController(effect_runners);
    }
    nonblock(NB_DISABLE);

    return 0;
}
