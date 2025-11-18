#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <artnet.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_color_t;

typedef struct {
    uint8_t brightness; // 0-255
    led_color_t color;
} led_state_t;

typedef struct {
    uint16_t id;
    uint8_t universe;
    uint16_t dmx_address;
    led_state_t state;
} led_t;

typedef struct {
    led_t *leds;
    uint16_t led_count;
    uint8_t start_universe;
    uint16_t start_address;
    uint8_t brightness;
} led_strip_t;

void initialize_led(led_t *led, uint16_t id, uint8_t universe, uint16_t dmx_address);
void initialize_led_strip(led_strip_t *strip);
void set_led_color(led_t *led, uint8_t r, uint8_t g, uint8_t b);
void set_led_color_from_hex(led_t *led, uint32_t hex_color);
void set_led_brightness(led_t *led, uint8_t brightness);
void set_dmx_data(led_t *led, uint8_t dmx_data[][DMX_DATA_MAX_LENGTH], uint8_t num_universes);
void set_strip_dmx_data(led_strip_t *strip, uint8_t dmx_data[][DMX_DATA_MAX_LENGTH], uint8_t num_universes);

#endif // LED_H