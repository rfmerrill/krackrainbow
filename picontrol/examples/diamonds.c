unsigned char pixel_color(int x, int y, int frame, int color) {
  if (x >= 24) {
    x = x - 24;
  }
 
  if (x >= 12) {
    x = 23 - x;
  } 
  
  if (y > 11) {
    y = 23 - y;
  }

  x = x + y;

  if (((frame + x)%7) & (1<<color)) {
    return 0xFF;
  } else {
    return 0x0;
  }
}
