#include <stdio.h>
#include <stdlib.h>

// This file is meant to be a framework for making cute little demos on krackrainbow
// It's not the best way to do all things for sure, but with this template you can do
// most, I'd expect.

// Define this function in a separate file and link it with this one
unsigned char pixel_color(int x, int y, int framenum, int color);
// Then, pipe the output of this program into kracksplit/krackoutput


static const char *conv_str = "0123456789ABCDEF";

void output_frame (unsigned char address, unsigned char frame[192]) {
  int i = 0;

  printf("%02X", address);

  for (i = 0; i < 192; i++) { putchar(conv_str[frame[i] % 16]); }

  putchar('\n');

  fflush(stdout);
}

#define FIRST_X(addr) (8*((addr-13)%6))
#define FIRST_Y(addr) (8*((addr-13)/6))

int main(int argc, char **argv) {
  int i = 0, j = 0, addr = 0;;
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
        frame[i] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) + (i/8), count, 0); // get red value for pixel
        frame[i+64] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) + (i/8), count, 1); // get green value
        frame[i+128] = pixel_color(FIRST_X(addr) + (i%8), FIRST_Y(addr) +(i/8), count, 2); // get blue value
      }

      output_frame(addr, frame);
    }

    usleep(delay);
  }

}
