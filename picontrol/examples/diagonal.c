/*
Author: Robert "Finny" Merrill
*/

unsigned char pixel_color(int x, int y, int frame, int color) {
  if ((((frame / 16) % 7) + 1) & (1<<color))
    return 0;
    
  if (((y % 8) - (x % 8) + 8) != (frame % 16))
    return 0;
    
  return 0xF;
}
