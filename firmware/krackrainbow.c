/*
 * krackrainbow firmware, Copyright 2013 Robert "Finny" Merrill <rfmerrill@berkeley.edu>
 *
 * Originally based on the neorainbow firmware by michael vogt <michu@neophob.com>,
 * which was originally based on a bunch of other things.
 *
 * krackrainbow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * krackrainbow is distributed in the hope that it will be useful,
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


// see rainbow.h
volatile uint8_t buffer_status[NUM_BUFFERS] = {0};

uint8_t g_line,g_level;     // current row + current brightness level
uint8_t g_bufCurr;          // buffer we are /currently/ rendering from
uint8_t g_bufNext;          // buffer that we will switch to when done

uint8_t g_circle;           // basically just g_line * g_level, saves cycles

uint8_t auto_advance = 1;   // Will mean something in the future.


// This timer "ticks" at 2 MHz, so a CYCLE_TIME of 200
// means that it will fire every 1600 clock cycles,
// or 10 kHz. It takes 8 * 16 = 128 rendering cycles to
// draw a frame, so this gives us a frame rate of
// approximately 80 FPS.

#define CYCLE_TIME 200

void timer_init() {
  TCCR1B |= (1<<CS11) | (1<<WGM12); // Enable timer @ 1/8th CPU speed, Clear on compare
  OCR1A = CYCLE_TIME;               // Timer will reset + fire interrupt when it reaches this value
}


// We don't have to worry about the TWI clock because we're
// only a slave. Could probably swap out the clock crystal for
// A bigger one and it would still work fine.

// Squeezing the last of the bandwidth out of TWI was certainly
// an exercise! See twi_vect.c

void twi_init() {
  input_index = 0;
  input_buffer = 1;

  TWAR = SLAVE_ADDRESS << 1;
  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
}




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

    // Change the timer frequency to give us a better range of brightness
    // When it's linear the brightness levels got more same-y toward the top

    if (g_level < 6)
      OCR1A = 100;
    else if (g_level < 11)
      OCR1A = 200;
    else
      OCR1A = 300;
  }

  g_circle++;

  if (g_circle == CIRCLE) {
    // It appears we've completed a whole frame! Now's the time to decide
    // if we're going to switch buffers.

    buffer_status[g_bufCurr] = BUFFER_CLEAN;
    buffer_status[g_bufNext] = BUFFER_BUSY;
    g_bufCurr = g_bufNext;
  }

  // Send stuff out the shift register.
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

        displayNextLine();

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
    }
  }

}
