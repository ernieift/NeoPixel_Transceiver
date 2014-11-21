/**
 ******************************************************************************
 * @file       NeoPixel_Transceiver.ino
 * @author     ernieift, Copyright (C) 2014
 * @brief      protocol converter to use rgb led stripes at async serial ports
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* pinout used on ATtiny45
 * pin 1 -
 * pin 2 RX comes from host
 * pin 3 TX (not used at yet)
 * pin 4 GND
 * pin 5 DOUT goes to LED stripe
 * pin 6 -
 * pin 7 -
 * pin 8 VCC
 */

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
      
