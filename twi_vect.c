
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/twi.h>

#include "rainbow.h"

#define TWI_FREQ 400000UL
#define F_CPU 16000000UL

//#define SLAVE_ADDRESS 112

void twi_otherstuff(void);

// Globally reserve some registers so we can avoid pushing them.

register uint8_t input_index asm ("r2");
register uint8_t input_limit asm ("r3");
register uint8_t twsr_reg asm("r16");
register uint8_t twdr_reg asm("r17");

extern unsigned char buffer[3][96];
extern volatile uint8_t g_swapNow;
extern uint8_t g_bufCurr;

static uint8_t frameskip;

ISR(TWI_vect) {

  twsr_reg = TWSR;
  twsr_reg &= TW_STATUS_MASK;
  twdr_reg = TWDR;

//  TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);

  if (__builtin_expect(!!(twsr_reg == TW_SR_DATA_ACK), 1)) {
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);
  }

  switch (__builtin_expect(twsr_reg,TW_SR_DATA_ACK)) {
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

      if (input_index < input_limit) {
        buffer[0][input_index] = twdr_reg;
        input_index++;
      }

      break;

    case TW_SR_STOP:

      g_swapNow = !frameskip;


      break;
  }
  if (twsr_reg != TW_SR_DATA_ACK)
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);
}

