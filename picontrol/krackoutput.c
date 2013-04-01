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
  char buffer[1024] = {0};
  unsigned char message[512] = {0};

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


  while (!feof(stdin) && !ferror(stdin)) {
    int c;
    char *pc = buffer;
    int idx = 0;
    int odd = 0;

    fgets(buffer, sizeof buffer, stdin);

    while (*pc != '\0') {
      c = hexdigit(*pc++);

      if (c == 255) // Not a hex digit, skip
        continue;

      if (odd) {
        message[idx] |= c;
        idx++;
      } else {
        message[idx] = c << 4;
      }

      odd = !odd;
    }

    if (idx >= 2) {
      devAddr = message[0];
      ioctl(fd, I2C_SLAVE, devAddr);
      write(fd, message + 1, idx-1);
    }
  }

  close(fd);
  return 0;
}
