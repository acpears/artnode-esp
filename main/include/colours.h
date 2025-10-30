#ifndef COLOURS_H
#define COLOURS_H
#include <stdint.h>

#define RED     0xFF0000
#define GREEN   0x00FF00
#define BLUE    0x0000FF
#define YELLOW  0xFFFF00
#define CYAN    0x00FFFF
#define MAGENTA 0xFF00FF
#define WHITE   0xFFFFFF
#define BLACK   0x000000

void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);
void hex_to_rgb(uint32_t hex, uint8_t *r, uint8_t *g, uint8_t *b);

#endif // COLOURS_H