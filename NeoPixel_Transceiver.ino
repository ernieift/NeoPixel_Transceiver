

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

/* global definition */
#define TIMEMAX 3

/* Serial definition */
#define RXPIN 3
#define TXPIN 4
SoftwareSerial softSerial(RXPIN, TXPIN);

/* LED definition */
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
#define LEDPIN 0
#define LEDMAX 100
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDMAX, LEDPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  softSerial.begin(9600);
}

void loop() {
  enum machinestates {
    STATE_IDLE=0,
    STATE_RECEIVE_R=1,
    STATE_RECEIVE_G=2,
    STATE_RECEIVE_B=3,
    STATE_SEND=4
  };

  uint8_t oldState, newState;
  uint8_t timeout;

  struct {
    uint16_t n;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } led;

  oldState = STATE_IDLE;
  newState = STATE_IDLE;
  timeout = 0;

  while (1) {
    if (oldState == STATE_IDLE) {
      if (softSerial.available()) {
        led.n = 0;
        newState = STATE_RECEIVE_R;
      }
      timeout = 0;
    }
    else if (oldState == STATE_RECEIVE_R) {
      if (softSerial.available()) {
        led.r = softSerial.read();
        newState = STATE_RECEIVE_G;
      }
    }
    else if (oldState == STATE_RECEIVE_G) {
      if (softSerial.available()) {
        led.g = softSerial.read();
        newState = STATE_RECEIVE_B;
      }
    }
    else if (oldState == STATE_RECEIVE_B) {
      if (softSerial.available()) {
        led.b = softSerial.read();
        strip.setPixelColor(led.n, led.r, led.g, led.b);
        led.n++;
        if (led.n < LEDMAX)
          newState = STATE_RECEIVE_R;
        else
          newState = STATE_SEND;
      }
    }
    else if (oldState == STATE_SEND) {
      strip.show();
      delay(1);
      newState = STATE_IDLE;
    }
    
    if (newState != oldState) {
      timeout = 0;
    }
    else {
      delay(1);
      timeout++;
      if (timeout >= TIMEMAX) {
        newState = STATE_SEND;
      }
    }

    oldState = newState;
  }
}
      
