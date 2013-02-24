#ifndef Rainbow_h
#define Rainbow_h


// Pins on the shift registers.

// Serial in on the first one in the chain
#define SH_BIT_SDI   0x01

// Clock for all three
#define SH_BIT_CLK   0x02

// Latch enable (inverted!!) for all three
#define SH_BIT_LE    0x04

// Output enable (NOT inverted!!) for all three
#define SH_BIT_OE    0x08


//potential take too long! -> PORTC &=~0x02; PORTC|=0x02
//Clock input terminal for data shift on rising edge
#define CLK_RISING  {PORTC &=~ SH_BIT_CLK; PORTC |= SH_BIT_CLK;}

#define SHIFT_DATA_1     {PORTC |= SH_BIT_SDI;}
//potential take too long! -> PORTC&=~0x01
#define SHIFT_DATA_0     {PORTC &=~ SH_BIT_SDI;}




register uint8_t input_index asm ("r2");
register uint8_t input_buffer asm ("r3");
register uint8_t twsr_reg asm("r16");
register uint8_t twdr_reg asm("r17");

#define NUM_BUFFERS 3

extern unsigned char buffer[3][96];

#define BUFFER_CLEAN        0
#define BUFFER_HAS_FRAME    1
#define BUFFER_HAS_COMMAND  2
#define BUFFER_BUSY         3

extern volatile uint8_t buffer_status[NUM_BUFFERS];
extern uint8_t g_bufCurr;
extern uint8_t input_buffer;


#endif

