/*
 * rainbowduino firmware, Copyright (C) 2010-2011 michael vogt <michu@neophob.com>
 *
 * based on
 * -blinkm firmware by thingM
 * -"daft punk" firmware by Scott C / ThreeFN
 * -rngtng firmware by Tobias Bielohlawek -> http://www.rngtng.com
 *
 * needed libraries:
 * -FlexiTimer (http://github.com/wimleers/flexitimer2)
 *
 * libraries to patch:
 * Wire:
 *  	utility/twi.h: #define TWI_FREQ 400000L (was 100000L)
 *                     #define TWI_BUFFER_LENGTH 98 (was 32)
 *  	wire.h: #define BUFFER_LENGTH 98 (was 32)
 *
 *
 * This file is part of neorainbowduino.
 *
 * neorainbowduino is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * neorainbowduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/twi.h>

#include "rainbow.h"

#define TWI_FREQ 400000UL
#define F_CPU 16000000UL

//#define SLAVE_ADDRESS 112


/*
A variable should be declared volatile whenever its value can be changed by something beyond the control
 of the code section in which it appears, such as a concurrently executing thread. In the Arduino, the
 only place that this is likely to occur is in sections of code associated with interrupts, called an
 interrupt service routine.
 */

volatile uint8_t buffer_status[NUM_BUFFERS] = {0};

uint8_t g_line,g_level;

uint8_t g_bufCurr;
uint8_t g_bufNext;

uint8_t g_circle;

uint8_t auto_advance = 1;


#define BRIGHTNESS_LEVELS 16
#define LED_LINES 8
#define CIRCLE BRIGHTNESS_LEVELS*LED_LINES


void select_line();
void shift_24_bit();


// We're basically doing manual PWM here. we display all lines
// for each brightness level and then go on to the next one.

// Since our shift registers are nice enough to have a latch
// enable line, we can clock bits in at our leisure and then
// trigger it

void displayNextLine() {
  // First things first: latch in the values we clocked in
  // last time.

  PORTC |= SH_BIT_OE; // Turn off the LED drivers (inverted)
  PORTC |= SH_BIT_LE;  // latch the shift register
  PORTC &= ~SH_BIT_LE; // un-latch

  // Pick our line
  select_line();

  PORTC &= ~SH_BIT_OE; // Turn the LED drivers back on.


  // Great, that's done, now we can prepare the next line

  // interlace lines, probably looks better.

  g_line+=2;

  if(g_line==LED_LINES) {
    g_line=1;
  }

  // on to the next brightness level.

  if(g_line>LED_LINES) {
    g_line=0;
    g_level++;
    if (g_level>=BRIGHTNESS_LEVELS) {
      g_level=0;
    }

    // exponential brightness, looks way better
    if (g_level < 6)
      OCR1A = 100;
    else if (g_level < 11)
      OCR1A = 200;
    else
      OCR1A = 300;
  }
  g_circle++;

  if (g_circle==CIRCLE) {							// check end of circle - swap only if we're finished drawing a full frame!

    buffer_status[g_bufCurr] = BUFFER_CLEAN;
    buffer_status[g_bufNext] = BUFFER_BUSY;
    g_bufCurr = g_bufNext;


    g_circle = 0;
  }



  //
  shift_24_bit();
}


// We don't care about the other pins on ports
// B and D (UART, SPI and an unused pin)
// so we can cheat here.

// Had the board design not insisted on keeping
// the UART, SPI /and/ I2C pins free, we could
// have just used one port for this whole thing.

// Also could have done the interlacing in hardware
// and then just used the port register to keep track
// of what line we're on, shifting it left (or right)
// one bit every time.

void select_line() {
  if (g_line < 3) {
    PORTD = 0;
  } else {
    PORTB = 0;
  }

  switch (g_line) {
    case 0: {
      PORTB = 0x04;
      break;
    }
    case 1: {
      PORTB = 0x02;
      break;
    }
    case 2: {
      PORTB = 0x01;
      break;
    }
    case 3: {
      PORTD = 0x80;
      break;
    }
    case 4: {
      PORTD = 0x40;
      break;
    }
    case 5: {
      PORTD = 0x20;
      break;
    }
    case 6: {
      PORTD = 0x10;
      break;
    }
    case 7: {
      PORTD = 0x08;
      break;
    }
  }
}




// display one line by the color level in buffer
void shift_24_bit() {
  uint8_t color,row,data0,data1,ofs;

  for (color=0;color<3;color++) {	           	//Color format GRB
    ofs = color*32+g_line*4;				//calculate offset, each color need 32bytes

    for (row=0;row<4;row++) {

      data1=buffer[g_bufCurr][ofs]&0x0f;                //get pixel from buffer, one byte = two pixels
      data0=buffer[g_bufCurr][ofs]>>4;
      //data0 = do_display & 0x0f;
      //data1 = do_display & 0x0f;
      ofs++;

      if(data0>g_level) { 	//is current pixel visible for current level (=brightness)
        SHIFT_DATA_1		//send high to the MBI5168 serial input (SDI)
      }
      else {
        SHIFT_DATA_0		//send low to the MBI5168 serial input (SDI)
      }
      CLK_RISING		//send notice to the MBI5168 that serial data should be processed

      if(data1>g_level) {
        SHIFT_DATA_1		//send high to the MBI5168 serial input (SDI)
      }
      else {
        SHIFT_DATA_0		//send low to the MBI5168 serial input (SDI)
      }
      CLK_RISING		//send notice to the MBI5168 that serial data should be processed
    }
  }
}


#define CYCLE_TIME 200

void timer_init() {
  TCCR1B |= (1<<CS11) | (1<<WGM12); // Enable timer @ 1/8th CPU speed, CTC
  OCR1A = CYCLE_TIME;  // Timer will reset + fire interrupt when it reaches this value
}



void twi_init() {
  input_index = 0;
  input_buffer = 1;

  TWAR = SLAVE_ADDRESS << 1;
  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
}


int main(void) {
  int i, j;

  DDRD=0xff;        // Configure ports (see http://www.arduino.cc/en/Reference/PortManipulation): digital pins 0-7 as OUTPUT
  DDRC=0xff;        // analog pins 0-5 as OUTPUT
  DDRB=0xff;        // digital pins 8-13 as OUTPUT
  PORTD=0;          // Configure ports data register (see link above): digital pins 0-7 as READ
  PORTB=0;          // digital pins 8-13 as READ

  g_level = 0;
  g_line = 0;
  g_bufCurr = 0;
  g_bufNext = 1;
  g_circle = 0;

  auto_advance = 1;

  buffer_status[0] = BUFFER_HAS_FRAME;
  buffer_status[1] = BUFFER_HAS_FRAME;
  buffer_status[2] = BUFFER_HAS_FRAME;

  timer_init();
  twi_init();
  sei();

  PORTD &= ~(1<<2);

  while (1) {
    if (TIFR1 & _BV(OCF1A)) {

        TIFR1 |= _BV(OCF1A);

        PORTD |= (1<<2);
        displayNextLine();
        PORTD &= ~(1<<2);

        // Don't handle commands just yet

        for (i = 0; i < NUM_BUFFERS; i++) {
          if (buffer_status[i] == BUFFER_HAS_COMMAND)
            buffer_status[i] = BUFFER_CLEAN;
        }


        // Find the first buf after bufCurr which has data for us.
        // If none, keep bufCurr
        // If we've already found one last time around,
        // don't waste cycles

        if (auto_advance && (g_bufNext == g_bufCurr)) {
            g_bufNext++;

            if (g_bufNext == NUM_BUFFERS)
                g_bufNext = 0;

            while ((buffer_status[g_bufNext] != BUFFER_HAS_FRAME) &&
                   (g_bufNext != g_bufCurr)) {

                g_bufNext++;

                if (g_bufNext == NUM_BUFFERS)
                  g_bufNext = 0;
            }
        }

        PORTD |= (1<<2);
        PORTD &= ~(1<<2);
    }
  }

}
