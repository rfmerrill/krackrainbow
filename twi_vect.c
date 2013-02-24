
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/twi.h>

#include "rainbow.h"

#define TWI_FREQ 400000UL
#define F_CPU 16000000UL

//#define SLAVE_ADDRESS 112

void twi_otherstuff(void);

register uint8_t input_index asm ("r2");
register uint8_t input_limit asm ("r3");
extern uint8_t g_bufCurr;

extern unsigned char buffer[2][96];


ISR(TWI_vect) {
   uint8_t the_byte;


  if (TW_STATUS == TW_SR_DATA_ACK) {
    the_byte = TWDR;
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE) | _BV(TWINT);

    if (input_index < input_limit) {
      buffer[0][input_index] = the_byte;
      input_index++;
    }

  } else {
    twi_otherstuff();
  }

}
