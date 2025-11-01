#include "led.h"

#include <stdint.h>

void initialize_led(led_t *led, uint16_t id, uint8_t universe, uint16_t dmx_address) {
    led->id = id;
    led->universe = universe;
    led->dmx_address = dmx_address;
    led->state.brightness = 255; // Default to off
    led->state.color.r = 0;
    led->state.color.g = 0;
    led->state.color.b = 0;
}

void initialize_led_strip(led_strip_t *strip){
    for (uint16_t i = 0; i < strip->led_count; i++) {
        initialize_led(&strip->leds[i], i + 1, strip->start_universe, strip->start_address + (i * 3) % DMX_DATA_MAX_LENGTH);
    }
}

void set_led_color(led_t *led, uint8_t r, uint8_t g, uint8_t b) {
    led->state.color.r = r;
    led->state.color.g = g;
    led->state.color.b = b;
}

void set_led_color_from_hex(led_t *led, uint32_t hex_color) {
    // Extract RGB components from hex color
    uint8_t r = (hex_color >> 16) & 0xFF;
    uint8_t g = (hex_color >> 8) & 0xFF;
    uint8_t b = hex_color & 0xFF;

    // Set the LED color
    led->state.color.r = r;
    led->state.color.g = g;
    led->state.color.b = b;
}

void set_led_brightness(led_t *led, uint8_t brightness) {
    led->state.brightness = brightness;
}

void set_dmx_data(led_t *led, uint8_t dmx_data[][DMX_DATA_MAX_LENGTH], uint8_t num_universes) {

    uint8_t universe = led->universe - 1;
    uint16_t address = led->dmx_address - 1; // DMX addresses are 1-based

    // Bounds check to prevent array overflow
    if (universe >= num_universes) {
        return;  // Ignore if universe is out of range
    }

    if (address > DMX_DATA_MAX_LENGTH - 3) {  // Need 3 consecutive channels (R, G, B)
        return;  // Ignore if address is out of range
    }

    // Use proper scaling with rounding
    // Cast to uint16_t to prevent overflow, then divide
    dmx_data[universe][address]     = ((uint16_t)led->state.color.r * led->state.brightness + 127) / 255;
    dmx_data[universe][address + 1] = ((uint16_t)led->state.color.g * led->state.brightness + 127) / 255;
    dmx_data[universe][address + 2] = ((uint16_t)led->state.color.b * led->state.brightness + 127) / 255;
}

void set_strip_dmx_data(led_strip_t *strip, uint8_t dmx_data[][DMX_DATA_MAX_LENGTH], uint8_t num_universes) {
    for (uint16_t i = 0; i < strip->led_count; i++) {
        set_dmx_data(&strip->leds[i], dmx_data, num_universes);
    }
}