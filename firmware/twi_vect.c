/*
 * krackrainbow firmware, Copyright 2013 Robert "Finny" Merrill <rfmerrill@berkeley.edu>
 *
 * This file is entirely my own work and it is released under the WTFPL
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/twi.h>

#include "rainbow.h"

// Here's where the fun is.
// I did not design these boards, so I did not make the decision to use
// TWI/I2C for inter-board communication. Out of TWI, SPI and serial,
// TWI was the /worst/ choice, not only because it's bidirectional
// and therefore difficult to get a good rise time from, but the
// ATMega328's TWI peripheral doesn't give you even a single byte of
// buffer, so every cycle between the interrupt triggering and
// TWINT being cleared means complete dead time on the WHOLE bus.

// Also, even running at "fast" speed, you can /maybe/ get 350 kbps. I haven't
// tried overclocking the I2C bus but I don't think I could squeeze much more
// out of it.

// If you're thinking about building a similar project, the AVR's UART has
// an addressing mode, and also supports synchronous clock. I was thinking
// of redoing it with this but the raspberry pi doesn't support 9 data bits,
// nor does it support particularly fast serial speeds. Plus, the clock pin
// was already in use and I didn't feel like doing board surgery.

// So we have to make this superfast. Hand-optimized assembly is forthcoming.


ISR(TWI_vect) {
  // Alright, so our challenge is to pull the status byte and the data byte
  // out and set TWCR as fast as humanly possible. Let's do it!

  // I'm mostly concerned with the data byte execution path because we have
  // 96 of those for every one or two of the others.

  // twsr_reg, twdr_reg, input_index and input_buffer, are declared as
  // globally reserved registers to reduce the number of registers used
  // and therefore the number of push instructions in the preamble.

  // avr-gcc pushes everything at the beginning of the function, and it's
  // surprisingly difficult to convince it to stick anything before that.
  // it's also surprisingly difficult to get it to actually use the dedicated
  // registers you tell it to.

  // If you do any of this in C, GCC allocates additional registers for no
  // good reason.

  asm ("lds   r16, 185");     // twsr_reg = TWSR
  asm ("lds   r17, 187");     // twdr_reg = TWDR
  asm ("andi  r16, lo8(-8)"); // twsr_reg &= TW_STATUS_MASK

  // This uses a register, but GCC is generally nice enough to reuse it later.
  // TWCR is in the "extended" io space so the sbi/cbi instructions don't work.
  // For some reason it doesn't like it if you clear TWINT for conditions other than
  // data received.
  if (__builtin_expect(!!(twsr_reg == TW_SR_DATA_ACK), 1)) {
    asm("nop");  // We are apparently too fast!
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);

    // If we want to save more registers and more cycles, we could
    // try 256-byte-aligning each of the three buffers...
    // Dunno if it's worth the effort.

      if (input_index < 96) {
        buffer[input_buffer][input_index] = twdr_reg;
        input_index++;
      }

    return;
  }


  // So with the above I've managed to convince it to only push four
  // registers, and to set TWINT at damn near the beginning of the
  // function.

  // Maybe we could get rid of some of these options. Are we gonna use
  // general call? Hmm... I don't think we can lose arbitration when we've
  // never been a master....

  switch (twsr_reg) {
    case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
      // For now, do the same thing as normal. I dunno if we'll
      // treat general calls differently in the future.

      if (input_index < 96) {
        buffer[input_buffer][input_index] = twdr_reg;
        input_index++;
      }

      break;


    case TW_SR_SLA_ACK:   // addressed, returned ack
    case TW_SR_GCALL_ACK: // addressed generally, returned ack
    case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
    case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
      // Beginning of a packet. Not much to do here.

      input_index = 0;
      break;


    case TW_SR_STOP:
      // End of a packet. If it's frame-sized, it's a frame
      // Anything else and it's not a frame.

       if (input_index == 96) {
         buffer_status[input_buffer] = BUFFER_HAS_FRAME;
       } else {
         buffer_status[input_buffer] = BUFFER_HAS_COMMAND;
       }

       // Maybe later do something if we outrun the main thread
       // but for now let's just depend on the sender not to do that.
       // We're triple-buffering, shouldn't be too hard.

       input_buffer++;
       if (input_buffer == NUM_BUFFERS) {
         input_buffer = 0;
       }

      break;
  }

  // TWCR |= _BV(TWINT) takes more cycles
  // and we already know what it should be set to anyway.

  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);
}

