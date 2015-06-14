/**
 *****************************************************************************
 * @file       NeoPixel_Transceiver.ino
 * @author     ernieift, Copyright (C) 2014/2015
 * @brief      protocol converter to use rgb led stripes at async serial ports
 * @brief      and/or as i2c slave
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

/*
 * pins on ATtiny85
 *              ------ 
 *  RESET  PB5 |1 \/ 8| VCC
 *         PB3 |2    7| PB2 SCL
 *         PB4 |3    6| PB1
 *         GND |4    5| PB0 SDA
 *              ------
 *
 * default wiring for softserial
 * RX <- PB3
 * TX -> PB4
 * 
 * default LED output
 * LED -> PB1
 *
 */

/**
 * main defines
 * used for pinout and communication protocol
 *
 * note: in i2c mode reads and writes goes directly to LED buffer
 * pay attantion to the byte order here
 */

/* Serial definition */
//#define COM_SERIAL
#define RXPIN 3
#define TXPIN 4
#define BAUDRATE 9600
#define TIMEMAX 3

/* I2C definition */
#define COM_I2C
#define I2C_SLAVE_ADDRESS 0x50 // the 7-bit address (use the same as 24LC01)

/* LED definition */
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
#define NUMLED 64 /* tested @ATTiny85 and i2c. for faster updates reduce this */
#define LEDPIN 1
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMLED, LEDPIN, NEO_GRB + NEO_KHZ800);

#if defined(COM_SERIAL) && defined(COM_I2C)
#error "can't use serial and i2c communication at the same time"
#undef COM_SERIAL
#undef COM_I2C
#endif

#ifdef COM_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial softSerial(RXPIN, TXPIN);
#endif

#ifdef COM_I2C
#include <TinyWireS.h>
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

static volatile uint8_t *i2c_regs;
static volatile uint8_t i2c_size;
static volatile uint8_t i2c_pos;

void i2c_requestEvent()
{
  if (i2c_pos < i2c_size) {
    TinyWireS.send(i2c_regs[i2c_pos]);
  } else {
    TinyWireS.send(0);
  }
  i2c_pos++;
}

void i2c_receiveEvent(uint8_t amount)
{
  // set register address
  if ((amount > 0) && (amount < TWI_RX_BUFFER_SIZE)) {
    i2c_pos = TinyWireS.receive();
    amount--;
  } else {
    return;
  }

  // isn't there anymore?
  if (amount == 0) {
    return;
  }
  
  while(amount--) {
    if ((i2c_regs != NULL) && (i2c_pos < i2c_size)) {
      i2c_regs[i2c_pos] = TinyWireS.receive();
    } else {
      TinyWireS.receive();
    }
    i2c_pos++;
  }
  strip.show();
}
#endif

void setup() {
  // initialize leds
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

#ifdef COM_SERIAL
  // initialize serial communication
  softSerial.begin(BAUDRATE);
#endif

#ifdef COM_I2C
  // initialize i2c communication
  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(i2c_receiveEvent);
  TinyWireS.onRequest(i2c_requestEvent);
 i2c_regs = strip.getPixels();
 i2c_size = strip.numPixels() * 3;
 i2c_pos = 0;
#endif
}

void loop() {
#ifdef COM_SERIAL
  // handle serial communication
  enum machinestates {
    STATE_IDLE=0,
    STATE_RECEIVE_R=1,
    STATE_RECEIVE_G=2,
    STATE_RECEIVE_B=3,
    STATE_SEND=4
  };

  uint8_t oldState, newState;
  uint8_t timeout;
  uint16_t n;
  uint8_t r,g,b;

  oldState = STATE_IDLE;
  newState = STATE_IDLE;
  timeout = 0;

  while (1) {
    switch (oldState) {
      case STATE_IDLE:
        if (softSerial.available()) {
          n = 0;
          newState = STATE_RECEIVE_R;
        }
        break;
      case STATE_RECEIVE_R:
        if (softSerial.available()) {
          r = softSerial.read();
          newState = STATE_RECEIVE_G;
        }
        break;
      case STATE_RECEIVE_G:
        if (softSerial.available()) {
          g = softSerial.read();
          newState = STATE_RECEIVE_B;
        }
        break;
      case STATE_RECEIVE_B:
        if (softSerial.available()) {
          b = softSerial.read();
          strip.setPixelColor(n, r, g, b);
          newState = (++n < NUMLED) ? STATE_RECEIVE_R : STATE_SEND;
        }
        break;
      case STATE_SEND:
        strip.show();
        newState = STATE_IDLE;
        break;
      default:
        newState = STATE_IDLE;
    }

    if ((newState != oldState) || (oldState == STATE_IDLE)) {
      timeout = 0;
    }
    else if (timeout >= TIMEMAX) {
      newState = STATE_SEND;
    }
    else {
      delay(1);
      timeout++;
    }

    oldState = newState;
  }
#endif

#ifdef COM_I2C
  // handle i2c communication.
  while (1) {
    TinyWireS_stop_check();
  }
#endif
}

