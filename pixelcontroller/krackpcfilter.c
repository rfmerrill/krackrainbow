#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define FRAME_SIZE (48*24*3)

#define FIRST_X(addr) (8*((addr-13)%6))
#define FIRST_Y(addr) (8*((addr-13)/6))

static const char *conv_str = "0123456789ABCDEF";

void output_frame (unsigned char address, unsigned char frame[192]) {
  int i = 0;

  printf("%02X", address);

  for (i = 0; i < 192; i++) { putchar(conv_str[frame[i] % 16]); }

  putchar('\n');

  fflush(stdout);
}

int main (int argc, char **argv) {
  int sockfd;
  struct sockaddr_in our_port = { 0 };
  unsigned int portnum;
  ssize_t packetlen;
  unsigned char packetbuf[4096];
  unsigned char frame[192];

  int count = 0, addr = 0, i = 0;

  if (argc < 2) {
    fprintf(stderr, "Too few arguments\n");
    return EXIT_FAILURE;
  }


  portnum = atoi(argv[1]);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    perror ("socket");
    return EXIT_FAILURE;
  }

  our_port.sin_family = AF_INET;
  our_port.sin_addr.s_addr = htonl(INADDR_ANY);
  our_port.sin_port = htons(portnum);

  if (bind(sockfd, (struct sockaddr *) &our_port, sizeof(our_port)) < 0) {
    perror ("bind");
    return EXIT_FAILURE;
  }

  while ((packetlen = recv(sockfd, packetbuf, sizeof packetbuf, 0)) > 0) {

   // printf ("Received packet #%d, size %d\n", count, packetlen);

    if (packetlen == FRAME_SIZE) {
      for (addr = 13; addr <= 30; addr++) {
        for (i = 0; i < 64; i++) {
          int myx = 47-(FIRST_X(addr) + (i%8));
          int myy = 23-(FIRST_Y(addr) + (i/8));

          frame[i+64] = packetbuf[3*(48*myy + myx)] >> 4;
          frame[i] = packetbuf[3*(48*myy + myx) + 1] >> 4;
          frame[i+128] = packetbuf[3*(48*myy + myx) + 2] >> 4;
        }

        output_frame(addr, frame);
      }
    }
    count++;
  }
  return 0;
}
