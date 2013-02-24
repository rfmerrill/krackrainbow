unsigned char pixel_color(int x, int y, int frame, int color) {
  if (x > 11) {
    if (x >= 36) {
      x = 47-x;
    } else {
      x = 11;
    }
  }
  
  if (y > 11) {
    y = 23 - y;
  }

  if (y < x) x = y;

  if (((frame + x)%7) & (1<<color)) {
    return 0xFF;
  } else {
    return 0x0;
  }
}
