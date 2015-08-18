/*

A sketch to take RPM data from a magnet and Hall Effect sensor and use that to
control a variable speed motor.

Also create a visualisation of the rpm using an LED strip.

16 Aug Commenting out tachometer code as we've decided it might distract from the main
    display. Ditto serial code as it's not required. Working version loaded to controller
    for the start of the LUX festival.



*/

#include <Adafruit_NeoPixel.h>
//#include <Wire.h>

// Settings for the tachometer
#define SENSOR_PIN    2  // INT0
#define LOW_RPM_THRESHOLD  3000 // milliseconds. Assumes the wheel is stopped if no ticks detected for this long.
#define WEIGHTING  10  // how much to weight the previous readings by. not used yet
#define EASTER_EGG_THRESHOLD  100  // how many cycles to over-rev to trigger the easter egg. not used yet

// Settings for the display
//#define SLAVE_ADDRESS    0x01 // the display's I2C address
//#define FORMAT_DECIMAL   0b01000000 // for sending to the display

// settings for the LED strip and motor control
#define MAX_RPM    511  // The maximum RPM from the bike that will define 'full power'
#define MIN_UPDATE  30  // minimum milliseconds between updating the LED strip. Affects how fast the strip 'moves'
#define MAX_UPDATE 1000 // maximum milliseconds between updates. ie, the slowest it will move
#define DASH_LENGTH  10 // the total number of dots that make a dash, includes blank ones
#define LIT_DASHES    7 // how many dots to light out of the total dash length
#define COLOUR_MAX  255 // the brightest to set the pixels

#define NUM_PIXELS  150 // how many pixels in the LED strip
#define LED_PIN     6   // which pin the LED strip data line ia connected to

// settings for the stepper motor
#define DIR_PIN    3
#define STEP_PIN   4
// Empirically derived from manually rotating the pot with a 10v supply.
#define MAX_VOLTAGE              10
#define MIN_VOLTAGE               0
#define MAX_VOLTAGE_READING    1020  // What the ADC reads at the max voltage required
#define MIN_VOLTAGE_READING       0  // Off
#define STEPPER_SPEED          0.05
#define VOLTAGE_PIN               0  // Analog read
#define JITTER_THRESHOLD          8  // Voltage readings jitter by a small amount. Ignore changes less than this
// Thresholds for the red -> yellow -> white fade
#define RED_THRESHOLD           150  // starts to go yellow after this many rpm
#define YELLOW_THRESHOLD        350  // starts to go white after this.

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_RGB Pixels are wired for RGB bitstream
// NEO_GRB Pixels are wired for GRB bitstream
// NEO_KHZ400 400 KHz bitstream (e.g. FLORA pixels)
// NEO_KHZ800 800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// volatile globals used by the sensor interrupt to calculate RPM
volatile unsigned long tick_time;
volatile unsigned long last_tick_time;
volatile int rpm;

void setup() {
  //Serial.begin(115200);
  // Stepper motor controller pins
  pinMode(DIR_PIN, OUTPUT); 
  pinMode(STEP_PIN, OUTPUT);
  setVoltage(0);  // make sure it's off when it comes up 
  //Wire.begin();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  attachInterrupt(INT0, ticker, FALLING); // pin goes low when sensor detects a magnetic field    
}

void loop() {   
  
    
    int voltage_reading;  // not actual voltage, but 0-1023 from the 10 bit ADC
    //float desired_voltage;  // is an actual voltage between MIN and MAX from defines above
    byte counter = 0;  // to keep track of drawing the dashes
    unsigned long next_push = 0;  // next time a pixel will be sent to the strip
    //byte over_rev = 0; // not used
    int avg_rpm = 0; // to smooth out the actual rpms a bit. Weighted average.
    int constrained_rpm;  // it's good to have boundaries

    while(1) {
      //avg_rpm = rpm; 
      avg_rpm = ((avg_rpm * WEIGHTING) + rpm)/(WEIGHTING + 1);  // weighted average
      constrained_rpm = constrain(avg_rpm, 0, MAX_RPM);
      setMotorSpeed(constrained_rpm);
      //sendCount(avg_rpm);
      //sendCount(analogRead(VOLTAGE_PIN));
      //Serial.println(millis());
      //Serial.println(next_push);
      if (millis() > next_push) {
        if (counter <= LIT_DASHES) {
            pushPixel(makeStripColour(constrained_rpm)); 
        } else {
            pushPixel(0,0,0);
        }
        next_push = millis() + MIN_UPDATE + (MAX_UPDATE - ((float)constrained_rpm/MAX_RPM * MAX_UPDATE));
        counter = (counter++) % DASH_LENGTH;
        /*
        Serial.print("AvgRPM: ");
        Serial.println(avg_rpm);
        Serial.print("ConstRPM: ");
        Serial.println(constrained_rpm);
        Serial.println(interval);
        Serial.println("----");
        */
      }
      if (millis() - last_tick_time > LOW_RPM_THRESHOLD) {
        // Setting RPM is done by triggering the sensor. If this
        // hasn't happened for a while then the wheel has stopped.
        rpm = 0;
      }
      delay(20); // so we don't spam the controller and display    
   }
}

void ticker() {
    // Triggered by a falling edge interrupt on pin 2 (INT0)
    // Use elapsed time in milliseconds since the last pulse to work out the RPM
    // This is accurate enough for the few hundred RPMs expected.
    tick_time = millis();
    rpm = (float)1000/(tick_time - last_tick_time) * 60;
    last_tick_time = tick_time;
}

void pushPixel(int r, int g, int b) {
    // Push a pixel of the given colour onto the first LED and shift all the rest up one.
    pushPixel(strip.Color(r,g,b));
}

void pushPixel(uint32_t colour) {
    // Push a pixel of the given colour onto the first LED and shift all the rest up one.
   
    for (int i = NUM_PIXELS-1; i>0; i--) {
        //Serial.println(i);
        strip.setPixelColor(i, strip.getPixelColor(i-1));
    }
    strip.setPixelColor(0, colour);
    //strip.setPixelColor(59, colour);
    strip.show();
}
    
void setMotorSpeed(int rpm) {
    // set the motor speed based on the supplied RPM
    // convert the rpm to voltage
    setVoltage(((float)rpm/MAX_RPM) * MAX_VOLTAGE); 
}

/* Tachometer. Not used.
void sendCount(int counter) {
    // send counter to display
    uint8_t bytes[5] = {FORMAT_DECIMAL, 0, 0, 0, 0}; // the display expects 5 bytes
    bytes[3] = counter/256;
    bytes[4] = counter % 256;
    Wire.beginTransmission(SLAVE_ADDRESS);
    for (int i=0;i<5;i++) {
        Wire.write((byte)bytes[i]);
        //Serial.print(bytes[i]);
        //Serial.print(" | ");
    }
    Wire.endTransmission();
    //Serial.print("\n");
}
*/

uint32_t makeStripColour(int revs) {
    // Take the RPM value and convert it to a compressed 32 bit colour for sending to the strip.
    // Fading up from red, through yellow to white using the following scheme
    // 0 - RED_THRESHOLD:  fade up red
    // RED to YELLOW:      fade in green proportionately to make yellow
    // YELLOW to MAX_RPM:  red & green go up together while bringing in blue 
  
    int r,g,b;
    
    if (revs < RED_THRESHOLD) {
        r = (float)revs/MAX_RPM * COLOUR_MAX;
        g = 0;
        b = 0;
    } else if (revs <= YELLOW_THRESHOLD) {
        r = (float)revs/MAX_RPM * COLOUR_MAX;
        g = (float)(revs - RED_THRESHOLD) / (YELLOW_THRESHOLD - RED_THRESHOLD) * ((float)YELLOW_THRESHOLD/MAX_RPM * COLOUR_MAX);
        b = 0;
    } else if (revs > YELLOW_THRESHOLD) {
        r = (float)revs/MAX_RPM * COLOUR_MAX;
        g = (float)revs/MAX_RPM * COLOUR_MAX;
        b = (float)(revs - YELLOW_THRESHOLD) / (MAX_RPM - YELLOW_THRESHOLD) * COLOUR_MAX;
    }
    /*
    Serial.println(revs);
    Serial.println(r);
    Serial.println(g);
    Serial.println(b);
    Serial.println("----");
    */
    return strip.Color(r,g,b);
}

// Stepper controller functions     
void setVoltage(float voltage) {
  // Seek the desired voltage
  // Convert from the actual voltage requested to the appropriate ADC reading
  //Serial.print("G: ");
  //Serial.println(voltage);
  voltage = voltage * ((float)MAX_VOLTAGE_READING/MAX_VOLTAGE);
  voltage = constrain(voltage,MIN_VOLTAGE_READING, MAX_VOLTAGE_READING);
  //Serial.print("Setting: ");
  //Serial.println(voltage);
  // only change if the current voltage reading is outside the expected jitter
  // or we're aiming for 0
  if (voltage != 0 && abs(analogRead(VOLTAGE_PIN) - voltage) > JITTER_THRESHOLD) {
    if (analogRead(VOLTAGE_PIN) < voltage) {
      while (analogRead(VOLTAGE_PIN) < voltage) {
        //Serial.print("UP: ");
        //Serial.println(analogRead(VOLTAGE_PIN));
        rotate(1, STEPPER_SPEED);
      }
    } else if (analogRead(VOLTAGE_PIN) > voltage) {
      while (analogRead(VOLTAGE_PIN) > voltage) {
        //Serial.print("DOWN: ");
        //Serial.println(analogRead(VOLTAGE_PIN));
        rotate(-1, STEPPER_SPEED);
      }
    }
  }
}

void rotate(int steps, float speed){ 
  //rotate a specific number of microsteps (8 microsteps per step) - (negative for reverse movement)
  //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
  int dir = (steps > 0)? HIGH:LOW;
  steps = abs(steps);

  digitalWrite(DIR_PIN,dir); 

  float usDelay = (1/speed) * 70;

  for(int i=0; i < steps; i++){ 
    digitalWrite(STEP_PIN, HIGH); 
    delayMicroseconds(usDelay); 

    digitalWrite(STEP_PIN, LOW); 
    delayMicroseconds(usDelay); 
  } 
}