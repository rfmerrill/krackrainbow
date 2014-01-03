/*
Author: Robert "Finny" Merrill
*/

#include <stdio.h>
#include <stdlib.h>

static const char *conv_str = "0123456789ABCDEF";

void output_frame(unsigned char address, unsigned char frame[192]) {
  int i = 0;
  
  printf("%02X", address);
  
  for (i = 0; i <192; i++) { putchar(conv_str[frame[i] % 16]); }
  
  putchar('\n');
  
  fflush(stdout);
  }
  
#define FIRST_X(addr) (8*((addr-13)%6))
#define FIRST_Y(addr) (8*((addr-13)/6))

int main(int argc, char **argv) {
  int i = 0, j = 0, addr = 0;;
  int count = 0;
  unsigned char frame[192] = {0};
  unsigned long delay;
  
  if (argc > 1) {
    delay = strtoul(argv[1], NULL, 0);
  } else {
    delay = 0;
  }
  
  for (;;) {
  for (addr = 13; addr <= 30; addr++) {
    for (i = 0; i < 64; i++) {
      frame[i] = 0;
      frame[i+64] = 0;
      frame[i+128] = 0;
    }
    
    //frame[((count / 16) % 3) * 64 + count % 16] = 0xF;
    
    frame[7 * (count % 2)] = 0xF;
    frame[64 + 7 * (count % 2)] = 0xF;
    frame[128 + 7 * (count % 2)] = 0xF;
    
    output_frame(addr, frame);
    }
    
    usleep(delay);
    count = (count + 1) % 60000;
  }
    
}
