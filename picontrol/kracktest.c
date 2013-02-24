#include <stdio.h>
#include <string.h>

int main(void) {
  int i, j;
  char outbuf[193];

  for (i = 0; i < 192; i++) {
    memset(outbuf, '0', 192);
    outbuf[192] = 0;

    outbuf[i] = 'F';

    for (j = 10; j <= 30; j++) {
      printf("%02X%s\n", j, outbuf);

    }
 
  }

  return 0;

}
