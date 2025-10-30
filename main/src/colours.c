#include "colours.h"
#include <math.h>

// Convert HSV color to RGB color
// h: Hue angle in degrees [0, 360)
// s: Saturation [0, 1]
// v: Value (brightness) [0, 1]
// 0°: Red 🔴
// 60°: Yellow 🟡
// 120°: Green 🟢
// 180°: Cyan 🩵
// 240°: Blue 🔵
// 300°: Magenta 🩷
// 360°: Back to Red 🔴

void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r_prime, g_prime, b_prime;
    
    if (h < 60) {
        r_prime = c; g_prime = x; b_prime = 0;
    } else if (h < 120) {
        r_prime = x; g_prime = c; b_prime = 0;
    } else if (h < 180) {
        r_prime = 0; g_prime = c; b_prime = x;
    } else if (h < 240) {
        r_prime = 0; g_prime = x; b_prime = c;
    } else if (h < 300) {
        r_prime = x; g_prime = 0; b_prime = c;
    } else {
        r_prime = c; g_prime = 0; b_prime = x;
    }
    
    *r = (uint8_t)((r_prime + m) * 255);
    *g = (uint8_t)((g_prime + m) * 255);
    *b = (uint8_t)((b_prime + m) * 255);
}

void hex_to_rgb(uint32_t hex, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (hex >> 16) & 0xFF;
    *g = (hex >> 8) & 0xFF;
    *b = hex & 0xFF;
}