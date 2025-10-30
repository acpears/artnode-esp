#include "patterns.h"
#include "colours.h"
#include "led.h"

#include <math.h>
#include <stdbool.h>

// Initialize pattern state
void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float speed, void *pattern_data) {
    state->pattern_id = pattern_id;
    state->time = 0.0f;
    state->speed = speed;
    state->active = true;
    state->pattern_data = pattern_data;
}

// Update single pattern
void update_pattern(pattern_state_t *state, pattern_func_t pattern_func, 
                   led_strip_t *strip, float delta_time) {
    if (!state->active) return;
    
    // Update pattern's internal time
    state->time += delta_time * state->speed;
    
    // Execute pattern
    pattern_func(strip, state);
}

// rainbow cycle pattern
// pattern_data expected to be float array: [color_width]
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state) {
    float color_width = state->pattern_data ? ((float *)state->pattern_data)[0] : 1.0f; // Full hue cycle
    for (uint16_t i = 0; i < strip->led_count; i++) {
        float hue = sinf((state->time * state->speed) + ((float)i / strip->led_count) * color_width * M_PI) * 180.0f + 180.0f;

        uint8_t r, g, b;
        hsv_to_rgb(hue, 1.0f, 1.0f, &r, &g, &b);
        
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], 255);
    }
}

// stroboscope pattern
// pattern_data expected to be float array: [hue, saturation, value, flash_on_off_ratio]
void stroboscope(led_strip_t *strip, pattern_state_t *state){
    float h, s, v;
    h = state->pattern_data ? ((float *)state->pattern_data)[0] : 0.0f;
    s = state->pattern_data ? ((float *)state->pattern_data)[1] : 1.0f;
    v = state->pattern_data ? ((float *)state->pattern_data)[2] : 1.0f;

    float flash_on_off_ratio = state->pattern_data ? ((float *)state->pattern_data)[3] : 0.5f; // Default 50% on/off
    float flash_duration = 1.0f / state->speed; // Duration of the flash in seconds
    float off_duration = flash_duration * flash_on_off_ratio;

    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    for (uint16_t i = 0; i < strip->led_count; i++) {
        set_led_color(&strip->leds[i], r, g, b);
        if (fmodf(state->time, flash_duration) < off_duration) {
            set_led_brightness(&strip->leds[i], 255);
        } else {
            set_led_brightness(&strip->leds[i], 0);
        }
    }
}

void fire(led_strip_t *strip, pattern_state_t *state) {
    float flicker_intensity = state->pattern_data ? ((float *)state->pattern_data)[0] : 1.0f;

    for (uint16_t i = 0; i < strip->led_count; i++) {
        // Calculate flicker based on time and LED index with a bit of randomness
        float random_offset = (float)(i * 37 % 100) / 100.0f * 2.0f * M_PI; // Random phase offset
        float flicker = (sinf(state->time * 10.0f + i + random_offset) + 1.0f) / 2.0f; // Normalize to 0-1
        flicker = powf(flicker, flicker_intensity); // Adjust intensity

        // Map flicker to color gradient from dark red to bright yellow
        uint8_t r = (uint8_t)(flicker * 255);

        uint8_t random_intensity = 56;
        int8_t random_green = (rand() % random_intensity); // Add some randomness to green channel
        uint8_t g = (uint8_t)(flicker * random_green);

        set_led_color(&strip->leds[i], r, g, 0);
        set_led_brightness(&strip->leds[i], 255);
    }
    // Fire pattern implementation goes here
}