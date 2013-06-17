#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static const char *conv_str = "0123456789ABCDEF";

void output_frame (unsigned char address, unsigned char frame[192]) {
  int i = 0;

  printf("%02X", address);

  for (i = 0; i < 192; i++) { putchar(conv_str[frame[i] % 16]); }

  putchar('\n');  // put a newline character at the end of the stream

  fflush(stdout); // flush the stream to standard out
}

#define FIRST_X(addr) (8*((addr-13)%6))
#define FIRST_Y(addr) (8*((addr-13)/6))

/*
  An array to count how many alive neighbors each pixel has.
  Format is: [frame number][horizontal location][vertical location]
*/
int neighbor_counts[2][48][24] = { 0 };
/*
  Boolean array showing whether pixels are alive.  So, alive[1][12][15] == 1
  says the pixel located at (x, y) = (12, 15) is alive in frame 1, and
  alive[0][42][02] == 2 means the pixel located at (x, y) = (42, 02) is dead
  in frame 0.
*/
int alive[2][48][24] = { 0 };

int current_frame = 0;

void step_sim() {
  int x = 0, y = 0;
  int next_frame;
  int is_alive, cnt;

  current_frame = current_frame % 2;  // use mod 2 arithmetic since we're only dealing with two frames
  next_frame = 1 - current_frame; // set next_frame to be whatever current_frame isn't in the Z_2 group

  memset(neighbor_counts[next_frame], 0, sizeof neighbor_counts[0]);
  memset(alive[next_frame], 0, sizeof alive[0]);

  for (x = 0; x < 48; x++) {  // for each column 0 to 47:
    for (y = 0; y < 24; y++) {  // for each row 0 to 23:
      is_alive = alive[current_frame][x][y];  // see if the pixel is alive
      cnt = neighbor_counts[current_frame][x][y]; // grab how many of its neighbors are alive

      if ((cnt == 3) || (is_alive && (cnt==2))) { // if 3 of its neighbors are alive or it's alive with 2 alive neighbors:
        /*
          Grab the left, right, up, and down pixels using mod 48 and mod 24 arithmetic:
        */
        int left = (x + 47) % 48;
        int right = (x + 49) % 48;
        int up = (y + 23) % 24;
        int down = (y + 25) % 24;

        alive[next_frame][x][y] = 1;  // set this pixel to alive
        /*
          Set this pixel's neighbor_counts, using the left, right, up, down, x, and y variables:
        */
        neighbor_counts[next_frame][left][up]++;
        neighbor_counts[next_frame][right][up]++;
        neighbor_counts[next_frame][left][down]++;
        neighbor_counts[next_frame][right][down]++;
        neighbor_counts[next_frame][left][y]++;
        neighbor_counts[next_frame][right][y]++;
        neighbor_counts[next_frame][x][up]++;
        neighbor_counts[next_frame][x][down]++;

      }
    }
  }

  current_frame = next_frame;
}


int main(int argc, char **argv) {
  int i = 0, j = 0, addr = 0;;
  int count = 10000;
  unsigned char frame[192] = {0};
  unsigned long delay;


  if (argc > 1) {
    delay = strtoul(argv[1], NULL, 0);
  } else {
    delay = 0;
  }

  srand(time(NULL));


  for (;;) {
    if (count > 2000) {
      count = 0;
      srand(time(NULL));

      for (i = 0; i < 48; i++) {
        for (j = 0; j < 24; j++) {
          alive[0][i][j] = rand()%2;
          neighbor_counts[0][i][j] = rand()%9;
        }
      }
    }
    count++;

    if (!(count % 2))
     step_sim();

    for (addr = 13; addr <= 30; addr++) {
      for (i = 0; i < 64; i++) {
        int xpos = FIRST_X(addr) + (i%8);
        int ypos = FIRST_Y(addr) + (i/8);

        int is_alive = alive[current_frame][xpos][ypos];
        int was_alive = alive[(1 - current_frame)][xpos][ypos];

        if (count % 2) {
          frame[i] = 0;
          frame[i+64] = 0;
          frame[i+128] = (is_alive) ? 0xF : 0;
        } else if (is_alive) {
          if (was_alive) {
            // blue
            frame[i] = 0;
            frame[64+i] = 0;
            frame[128+i] = 0xF;
          } else {
            // green
            frame[i] = 0xF;
            frame[64+i] = 0;
            frame[128+i] = 0;
          }
        } else {
          if (was_alive) {
            // red
            frame[i] = 0;
            frame[64+i] = 0xF;
            frame[128+i] = 0;
          } else {
            // black
            frame[i] = 0;
            frame[64+i] = 0;
            frame[128+i] = 0;
          }
        }
      }


      output_frame(addr, frame);

    }


    usleep(delay);
  }

}
