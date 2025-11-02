#include "patterns.h"
#include "colours.h"
#include "led.h"

#include <math.h>
#include <stddef.h>
#include <stdbool.h>

// Pattern registry for easy lookup by ID
const pattern_func_t pattern_functions[] = {
    fire,           // ID 0
    rainbow_cycle,  // ID 1
    stroboscope,    // ID 2
    solid_color,     // ID 3
    sparkle,        // ID 4
};

const char* pattern_names[] = {
    "Fire",
    "Rainbow Cycle", 
    "Stroboscope",
    "Solid Color",
    "Sparkle"
};

const uint8_t pattern_count = sizeof(pattern_functions) / sizeof(pattern_functions[0]);

// Helper function to get pattern function by ID
pattern_func_t get_pattern_function(uint16_t pattern_id) {
    if (pattern_id >= pattern_count) {
        return NULL;  // Invalid pattern ID
    }
    return pattern_functions[pattern_id];
}

// Initialize pattern state
void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float speed, void *pattern_data) {
    state->pattern_id = pattern_id;
    state->time = 0.0f;
    state->speed = speed;
    state->active = true;
    state->pattern_data = pattern_data;
}

// Update single pattern
void update_pattern(pattern_state_t *state, led_strip_t *strip, float delta_time) {
    if (!state->active) return;
    
    // Update pattern's internal time
    state->time += delta_time * state->speed;

    // Execute pattern function
    get_pattern_function(state->pattern_id)(strip, state);
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

// ID 2: fire effect
// pattern_data expected to be float array: [flicker_intensity, yellow_level (0-1]
void fire(led_strip_t *strip, pattern_state_t *state) {
    float flicker_intensity = state->pattern_data ? ((float *)state->pattern_data)[0] : 1.0f;
    float yellow_level = state->pattern_data ? ((float *)state->pattern_data)[1] : 0.0f;

    for (uint16_t i = 0; i < strip->led_count; i++) {
        // Calculate flicker based on time and LED index with a bit of randomness
        float random_offset = (float)(i * 37 % 100) / 100.0f * 2.0f * M_PI; // Random phase offset
        float flicker = (sinf((state->time * 5) + (i * 1.2f) + 1.0f) + 1.0f) / 2.0f; // Normalize to 0-1
        flicker = powf(flicker, flicker_intensity); // Adjust intensity

        // Map flicker to color gradient from dark red to bright yellow
        uint8_t r = (uint8_t)(flicker * 255);

        uint8_t green_default = 255 * yellow_level;
        uint8_t random_intensity = 10; // Random value between -4 and +4
        int8_t random_green = green_default +(rand() % random_intensity - (random_intensity / 2)); // Add some randomness to green channel
        uint8_t g = (uint8_t)(flicker * random_green);

        set_led_color(&strip->leds[i], r, g, 0);
        set_led_brightness(&strip->leds[i], 255);
    }
}

// ID 3: solid color pattern 
// pattern_data expected to be float array: [hue, saturation, value]
void solid_color(led_strip_t *strip, pattern_state_t *state) {
    float h, s, v;
    h = state->pattern_data ? ((float *)state->pattern_data)[0] : 0.0f;
    s = state->pattern_data ? ((float *)state->pattern_data)[1] : 1.0f;
    v = state->pattern_data ? ((float *)state->pattern_data)[2] : 1.0f;

    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    for (uint16_t i = 0; i < strip->led_count; i++) {
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], 255);
    }
}

// ID 4: Sparkle pattern
// pattern_data expected to be float array: [sparkle_chance(0-1), hue(0-360), saturation (0-1), background_hue(0-360), background_saturation(0-1), background_level(0-1)]
void sparkle(led_strip_t *strip, pattern_state_t *state) {
    float sparkle_chance = state->pattern_data ? ((float *)state->pattern_data)[0] : 0.5f; // Chance of sparkle per LED
    float sparkle_hue = state->pattern_data ? ((float *)state->pattern_data)[1] : 0.0f;
    float sparkle_saturation = state->pattern_data ? ((float *)state->pattern_data)[2] : 0.0f;
    float background_hue = state->pattern_data ? ((float *)state->pattern_data)[3] : 0.0f;
    float background_saturation = state->pattern_data ? ((float *)state->pattern_data)[4] : 0.0f;
    float background_level = state->pattern_data ? ((float *)state->pattern_data)[5] : 0.1f;
    bool inverted = false;

    uint8_t r, g, b;
    uint8_t background_brightness = (uint8_t)(background_level * 255);

    for (uint16_t i = 0; i < strip->led_count; i++) {
        float random_value = (float)(rand() % 1000) / 1000.0f; // Random value between 0 and 1
    
        if (random_value < log10f(sparkle_chance/20 + 1)) {
            hsv_to_rgb(sparkle_hue, sparkle_saturation, 1.0f, &r, &g, &b);
            set_led_color(&strip->leds[i], r, g, b);
            set_led_brightness(&strip->leds[i], inverted ? background_brightness : 255);
        } else {
            hsv_to_rgb(background_hue, background_saturation, 1.0f, &r, &g, &b);
            set_led_color(&strip->leds[i], r, g, b);
            set_led_brightness(&strip->leds[i], inverted ? 255 : background_brightness);
        }
    }
}