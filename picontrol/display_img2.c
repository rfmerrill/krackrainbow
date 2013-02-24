#include <stdio.h>


static const char *conv_str = "0123456789ABCDEF";

unsigned char pixel_color(int x, int y, int framenum, int color);

void output_frame (unsigned char address, unsigned char frame[192]) {
  int i = 0;

  printf("%02X", address);

  for (i = 0; i < 192; i++) { putchar(conv_str[frame[i] % 16]); }

  putchar('\n');

  fflush(stdout);  
}

#define FIRST_X(addr) (8*((addr-13)%6))
#define FIRST_Y(addr) (8*((addr-13)/6))

int main(void) {
  int i = 0, j = 0, addr = 0;;
  int count = 0;
  unsigned char frame[192] = {0};
    
  for (;;) {  
  for (addr = 13; addr <= 30; addr++) { 
    for (i = 0; i < 64; i++) {
      frame[i] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) + (i/8), count, 0);   
      frame[i+64] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) + (i/8), count, 1);   
      frame[i+128] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) +(i/8), count, 2);   
    }

    output_frame(addr, frame);
    }
    
    count = (count + 1) % 6000;
  }

}
