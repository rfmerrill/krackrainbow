
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/twi.h>

#include "rainbow.h"



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
      input_index = 0;
      break;

    case TW_SR_DATA_ACK:       // data received, returned ack
    case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack

      if (input_index < 96) {
        buffer[input_buffer][input_index] = twdr_reg;
        input_index++;
      }

      break;

    case TW_SR_STOP:

       if (input_index == 96) {
         buffer_status[input_buffer] = BUFFER_HAS_FRAME;
       } else {
         buffer_status[input_buffer] = BUFFER_HAS_COMMAND;
       }

       input_buffer++;
       if (input_buffer == NUM_BUFFERS) {
         input_buffer = 0;
       }

      break;
  }
  if (twsr_reg != TW_SR_DATA_ACK)
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);
}

