/*
 * kracksplit
 * A little utility I wrote for splitting a pixel stream into two instances of krackoutput,
 * for when you've got an array that's split across two I2C buses.
 * 
 * To run: kracksplit /path/to/krackoutput /dev/first-i2c-device /dev/second-i2c-device
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv) {
  FILE *p1, *p2;
  char *c1, *c2;
  size_t a1len;
  int c;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <program> <argforfirst> <argforsecond>\n", argv[0]);
    return EXIT_FAILURE;
  }

  a1len = strlen(argv[1]);

  c1 = malloc(a1len + strlen(argv[2]) + 2); // Two extra bytes, one for null terminator, one for space;
  c2 = malloc(a1len + strlen(argv[3]) + 2);

  if (!c1 || !c2) {
    fprintf(stderr, "Couldn't malloc\n");
    return EXIT_FAILURE;
  }

  strcpy(c1, argv[1]);
  strcpy(c2, argv[1]);

  c1[a1len] = ' ';
  c2[a1len] = ' ';
  
  strcpy(c1 + a1len + 1, argv[2]);
  strcpy(c2 + a1len + 1, argv[3]);

#ifndef NDEBUG 
  printf("c1: %s\nc2: %s\n", c1, c2);
#endif

  p1 = popen(c1, "w");

  if (!p1) {
    perror("popen");
    return EXIT_FAILURE;
  }

  p2 = popen(c2, "w");

  if (!p2) {
    perror("popen");
    return EXIT_FAILURE;
  }

  while ((c = getc(stdin)) != EOF) {
    putc(c, p1);
    putc(c, p2);
    if (c == '\n') {
      fflush(p1);
      fflush(p2);
    }
      
  }

  return 0;  
} 
