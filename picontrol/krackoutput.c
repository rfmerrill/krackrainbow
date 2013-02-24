/*
 * This little program is a tool to make it easier to get pixels onto the board
 * Basically, you pipe the pixels in an easy-to-handle text format into this, and 
 * it does the rest.

 * This program takes one argument, the path to the i2c device (usually /dev/i2c-0 and i2c-1)
 * If they aren't there, you might have to modprobe i2c-dev.
 */

#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


unsigned char hexdigit(char input) {
  input = toupper(input);
  if (input >= '0' && input <= '9') {
    return input - '0';
  } else if (input >= 'A' && input <= 'F') {
    return 10 + (input - 'A');
  }

  // no clue
  return 255;
}

int main(int argc, char **argv){
  int fd;
  long devAddr = 112;
  int i;

  struct timeval tv_begin, tv_end, tv_diff;
  char buffer[256] = {0};
  unsigned char message[96] = {0};
  
  if (argv[1]) {
    fd = open(argv[1], O_RDWR);
  } else {
    fprintf(stderr, "usage: %s [i2c device]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (!fd) {
    perror("open");
    return EXIT_FAILURE;
  }

#ifndef NDEBUG
  printf("Running with debug enabled\n");
#endif

  while (!feof(stdin) && !ferror(stdin)) {
    int c;

    fgets(buffer, 256, stdin);

    // String should be at least 194 characters long
    if (strlen(buffer) < 194) {
      // ignore
      continue;
    }


    // First two characters should be the hex address of
    // the target panel.

    c = hexdigit(buffer[0]);

    if (c == 255)
      continue;

    devAddr = hexdigit(buffer[1]);

    if (devAddr == 255)
      continue;

    devAddr |= (c << 4);

#ifndef NDEBUG
    printf ("Sending to address %lu (chars %c%c)\n", devAddr, buffer[0], buffer[1]);
#endif

    // The rest is stuff to send to the I2C bus   

    for (i = 0; i < 96; i++) {
      c = hexdigit(buffer[(2*i)+2]);

      if (c == 255) {
#ifndef NDEBUG
        printf("Character %c not recognized as hex digit\n", c);
#endif
        break;
      }

      message[i] = c << 4;

      c = hexdigit(buffer[(2*i)+3]);

      if (c == 255) {
#ifndef NDEBUG
        printf("Character %c not recognized as hex digit\n", c);
#endif
        break;
      }

      message[i] |= c;
    }

    if (c != 255) {
      ioctl(fd, I2C_SLAVE, devAddr);
      write(fd, message, 96);
    }
  }

  close(fd);
  return 0;
}
