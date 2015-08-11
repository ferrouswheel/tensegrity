#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef __ROTARY__
#else
#include "rotaryencoder.h"

int numberofencoders = 0;

unsigned int lastupdate = 0;

void updateEncoders()
{
    static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

    unsigned int now = micros();
    if (lastupdate - now < 400) {
        return;
    }
    lastupdate = now;

    struct encoder *encoder = encoders;
    for (; encoder < encoders + numberofencoders; encoder++)
    {
        int MSB = digitalRead(encoder->pin_a);
        int LSB = digitalRead(encoder->pin_b);

        int encoded = (MSB << 1) | LSB;
        int sum = (encoder->lastEncoded << 2) | encoded;

	//old_AB <<= 2;                   //remember previous state
	//old_AB |= ( ENC_PORT & 0x03 );  //add current state

	encoder->value += ( enc_states[( sum & 0x0f )]);

        encoder->lastEncoded = encoded;
    }
}

struct encoder *setupencoder(int pin_a, int pin_b)
{
    if (numberofencoders > max_encoders)
    {
        printf("Maximum number of encodered exceded: %i\n", max_encoders);
        return NULL;
    }
    lastupdate = micros();

    struct encoder *newencoder = encoders + numberofencoders++;
    newencoder->pin_a = pin_a;
    newencoder->pin_b = pin_b;
    newencoder->value = 0;
    newencoder->lastEncoded = 0;

    pinMode(pin_a, INPUT);
    pinMode(pin_b, INPUT);
    pullUpDnControl(pin_a, PUD_UP);
    pullUpDnControl(pin_b, PUD_UP);
    wiringPiISR(pin_a,INT_EDGE_BOTH, updateEncoders);
    wiringPiISR(pin_b,INT_EDGE_BOTH, updateEncoders);

    return newencoder;
}
#endif
