#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "patterns.h"
#include "colours.h"
#include "led.h"


// Update single pattern
void update_pattern(pattern_state_t *state, led_strip_t *strip, float delta_time) {
    if (!state->active) return;
    
    // Update pattern's internal time
    state->time += delta_time;

    // Execute pattern function
    get_pattern_function(state->pattern_id)(strip, state);
}

// PATTERNS

// Pattern function parameters
const pattern_param_t rainbow_cycle_params[] = {
    {"color_width", 1.0f, 0.1f, 5.0f, 1.0f, false, false},
    {"saturation", 1.0f, 0.0f, 1.0f, 1.0f, false, true},
    {"brightness", 1.0f, 0.0f, 1.0f, 1.0f, false, true}
};
const pattern_param_t stroboscope_params[] = {
    {"flash_on_off_ratio", 0.5f, 0.0f, 1.0f, 0.5f, false, false},
    {"hue", 0.0f, 0.0f, 360.0f, 0.0f, false, true},
    {"saturation", 1.0f, 0.0f, 1.0f, 1.0f, false, true},
    {"value", 1.0f, 0.0f, 1.0f, 1.0f, false, true}
};
const pattern_param_t fire_params[] = {
    {"flicker_intensity", 1.0f, 0.1f, 5.0f, 1.0f, false, false},
    {"yellow_level", 0.0f, 0.0f, 1.0f, 0.0f, false, true}
};
const pattern_param_t solid_color_params[] = {
    {"hue", 0.0f, 0.0f, 360.0f, 0.0f, false, true},
    {"saturation", 1.0f, 0.0f, 1.0f, 1.0f, false, true},
    {"value", 1.0f, 0.0f, 1.0f, 1.0f, false, true}
};
const pattern_param_t sparkle_params[] = {
    {"sparkle_chance", 0.5f, 0.0f, 1.0f, 0.5f, false, false},
    {"sparkle_hue", 0.0f, 0.0f, 360.0f, 0.0f, false, true},
    {"sparkle_saturation", 0.0f, 0.0f, 1.0f, 0.0f, false, true},
    {"sparkle_brightness", 1.0f, 0.0f, 1.0f, 0.0f, false, true},
    {"background_hue", 0.0f, 0.0f, 360.0f, 0.0f, false, true},
    {"background_saturation", 1.0f, 0.0f, 1.0f, 1.0f, false, true},
    {"background_brightness", 0.5f, 0.0f, 1.0f, 0.5f, false, true}
};

// ID 0:rainbow cycle pattern
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state) {
    float color_width = state->params[0].value;
    float saturation = state->params[1].value;
    uint8_t brightness = state->params[2].value * 255.0f;
    
    for (uint16_t i = 0; i < strip->led_count; i++) {
        float hue = sinf((state->time * state->speed) + ((float)i / strip->led_count) * color_width * M_PI) * 180.0f + 180.0f;

        uint8_t r, g, b;
        hsv_to_rgb(hue, saturation, 1.0f, &r, &g, &b);
        
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], brightness);
    }
}

// ID 1: stroboscope pattern
void stroboscope(led_strip_t *strip, pattern_state_t *state){
    float flash_on_off_ratio = state->params[0].value;
    float h, s, v;
    h = state->params[1].value * 360.0f;
    s = state->params[2].value;
    v = state->params[3].value;

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
void fire(led_strip_t *strip, pattern_state_t *state) {
    float flicker_intensity = state->params[0].value;
    float yellow_level = state->params[1].value;

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
void solid_color(led_strip_t *strip, pattern_state_t *state) {
    float h, s, v;
    h = state->params[0].value * 360.0f;
    s = state->params[1].value;
    v = state->params[2].value;

    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    for (uint16_t i = 0; i < strip->led_count; i++) {
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], 255);
    }
}

// ID 4: Sparkle pattern
void sparkle(led_strip_t *strip, pattern_state_t *state) {
    float sparkle_chance = state->params[0].value;
    float sparkle_hue = state->params[1].value * 360.0f;
    float sparkle_saturation = state->params[2].value;
    float sparkle_brightness = state->params[3].value * 255.0f;
    float background_hue = state->params[4].value * 360.0f;
    float background_saturation = state->params[5].value;
    float background_brightness = state->params[6].value * 255.0f;
    
    bool inverted = false;
    uint8_t r, g, b;

    // Create time-based sparkle persistence
    // Higher speed = faster changes, Lower speed = slower changes (longer lasting sparkles)
    float time_scale = state->time * state->speed;
    
    for (uint16_t i = 0; i < strip->led_count; i++) {
        // Create a time-based seed that changes based on speed
        // Each LED has a unique phase offset so they don't all change at once
        float led_phase = (float)i * 0.1f;
        float time_seed = time_scale + led_phase;
        
        // Use floor to create discrete time periods where sparkles persist
        float time_period = floorf(time_seed * 20.0f); // Change sparkle state 4 times per second at speed=1.0
        
        // Create a pseudo-random value that stays constant during each time period
        // This makes sparkles persist for the duration of the time period
        float seed = time_period + (float)i * 12.9898f; // Unique seed per LED and time period
        float pseudo_random = fmodf(sinf(seed) * 43758.5453f, 1.0f);
        if (pseudo_random < 0) pseudo_random += 1.0f; // Ensure positive
        
        // Apply sparkle chance threshold
        if (pseudo_random < sparkle_chance * 0.1f) {
            // This LED is sparkling during this time period
            hsv_to_rgb(sparkle_hue, sparkle_saturation, 1.0f, &r, &g, &b);
            set_led_color(&strip->leds[i], r, g, b);
            set_led_brightness(&strip->leds[i], inverted ? background_brightness : sparkle_brightness);
        } else {
            // This LED shows background color during this time period
            hsv_to_rgb(background_hue, background_saturation, 1.0f, &r, &g, &b);
            set_led_color(&strip->leds[i], r, g, b);
            set_led_brightness(&strip->leds[i], inverted ? sparkle_brightness : background_brightness);
        }
    }
}

// Pattern registry
const pattern_t pattern_registry[] = {
    {"Fire", fire, (pattern_param_t*)fire_params, 2},
    {"Rainbow Cycle", rainbow_cycle, (pattern_param_t*)rainbow_cycle_params, 3},
    {"Stroboscope", stroboscope, (pattern_param_t*)stroboscope_params, 4},
    {"Solid Color", solid_color, (pattern_param_t*)solid_color_params, 3},
    {"Sparkle", sparkle, (pattern_param_t*)sparkle_params, 7}
};

const uint8_t pattern_count = sizeof(pattern_registry) / sizeof(pattern_registry[0]);

// HELPER FUNCTIONS
char* get_pattern_name(uint8_t pattern_id) {
    if (pattern_id >= pattern_count) {
        return NULL;  // Invalid pattern ID
    }
    return pattern_registry[pattern_id].name;
}

pattern_func_t get_pattern_function(uint8_t pattern_id) {
    if (pattern_id >= pattern_count) {
        return NULL;  // Invalid pattern ID
    }
    return pattern_registry[pattern_id].func;
}

pattern_param_t* get_pattern_params(uint8_t pattern_id) {
    if (pattern_id >= pattern_count) {
        return NULL;  // Invalid pattern ID
    }
    return pattern_registry[pattern_id].params;
}

uint8_t get_pattern_param_count(uint8_t pattern_id) {
    if (pattern_id >= pattern_count) {
        return 0;  // Invalid pattern ID
    }
    return pattern_registry[pattern_id].param_count;
}

uint8_t get_total_pattern_count() {
    return pattern_count;
}   


