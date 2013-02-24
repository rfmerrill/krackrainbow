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

extern unsigned char buffer[3][96];  //two buffers (backbuffer and frontbuffer)

//interrupt variables
uint8_t g_line,g_level;

//read from bufCurr, write to !bufCurr
//volatile   //the display is flickerling, brightness is reduced
uint8_t g_bufCurr;

//flag to blit image
volatile uint8_t g_swapNow;
uint8_t g_circle;

//data marker
#define START_OF_DATA 0x10
#define END_OF_DATA 0x20

//FPS
#define FPS 80.0f

#define BRIGHTNESS_LEVELS 16
#define LED_LINES 8
#define CIRCLE BRIGHTNESS_LEVELS*LED_LINES

volatile uint8_t do_display;

volatile uint8_t current_index;
volatile uint8_t bus_error;



register uint8_t input_limit asm ("r3");
register uint8_t input_index asm ("r2");
register uint8_t twsr_reg asm("r16");
register uint8_t twdr_reg asm("r17");

void draw_next_line();
void shift_24_bit();
void open_line(unsigned char);

void displayNextLine() {
  draw_next_line();									// scan the next line in LED matrix level by level.
  g_line+=2;	 								        // process all 8 lines of the led matrix
  if(g_line==LED_LINES) {
    g_line=1;
  }
  if(g_line>LED_LINES) {								// when have scaned all LED's, back to line 0 and add the level
    g_line=0;
    g_level++;										// g_level controls the brightness of a pixel.
    if (g_level>=BRIGHTNESS_LEVELS) {							// there are 16 levels of brightness (4bit) * 3 colors = 12bit resolution
      g_level=0;
    }
    if (g_level < 6)
      OCR1A = 100;
    else if (g_level < 11)
      OCR1A = 200;
    else
      OCR1A = 300;
  }
  g_circle++;

  if (g_circle==CIRCLE) {							// check end of circle - swap only if we're finished drawing a full frame!

    cli();
    if (g_swapNow==1) {
      g_swapNow = 0;
      g_bufCurr = !g_bufCurr;
    }
    sei();

    g_circle = 0;
  }

  shift_24_bit();
}


// scan one line, open the scaning row
void draw_next_line() {
  DISABLE_OE						//disable MBI5168 output (matrix output blanked)
				//super source driver, select all outputs off

//  shift_24_bit();	// feed the leds
  LE_HIGH
  LE_LOW

  CLOSE_ALL_LINE
  open_line(g_line);

  ENABLE_OE							//enable MBI5168 output
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

void open_line(unsigned char line) {    // open the scaning line
  switch(line) {
  case 0: {
      open_line0
      break;
    }
  case 1: {
      open_line1
      break;
    }
  case 2: {
      open_line2
      break;
    }
  case 3: {
      open_line3
      break;
    }
  case 4: {
      open_line4
      break;
    }
  case 5: {
      open_line5
      break;
    }
  case 6: {
      open_line6
      break;
    }
  case 7: {
      open_line7
      break;
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
  input_limit = 0;

  TWAR = SLAVE_ADDRESS << 1;
  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
}

static uint8_t frameskip;

void twi_otherstuff() {
  switch (TW_STATUS) {
    case TW_SR_SLA_ACK:   // addressed, returned ack
    case TW_SR_GCALL_ACK: // addressed generally, returned ack
    case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
    case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
      input_index = (&buffer[!g_bufCurr][0] - &buffer[0][0]);;
      input_limit = input_index + 96;
      frameskip = 0;

      if (g_swapNow) {  // too soon! ignore this frame
        input_limit = input_index;
        frameskip = 1;
      }

      break;

    case TW_SR_DATA_ACK:       // data received, returned ack
    case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack

      break;

    case TW_SR_STOP:
      g_swapNow = !frameskip;
      break;
  }
  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);

}


int main(void) {

  DDRD=0xff;        // Configure ports (see http://www.arduino.cc/en/Reference/PortManipulation): digital pins 0-7 as OUTPUT
  DDRC=0xff;        // analog pins 0-5 as OUTPUT
  DDRB=0xff;        // digital pins 8-13 as OUTPUT
  PORTD=0;          // Configure ports data register (see link above): digital pins 0-7 as READ
  PORTB=0;          // digital pins 8-13 as READ

  g_level = 0;
  g_line = 0;
  g_bufCurr = 0;
  g_swapNow = 0;
  g_circle = 0;

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
        PORTD |= (1<<2);
        PORTD &= ~(1<<2);
    }
  }

}
