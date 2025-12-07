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

// Single solid color pattern
const pattern_param_t solid_color_params[] = {
    {"hue", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"value", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void solid_color(led_strip_t *strip, pattern_state_t *state) {
    float h, s, v;
    h = state->params[0].value;
    s = state->params[1].value / 100.0f; // Scale to 0 - 1
    v = state->params[2].value / 100.0f; 

    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    for (uint16_t i = 0; i < strip->led_count; i++) {
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], 255);
    }
}

// 2 solid color pattern
const pattern_param_t solid_color_params_2[] = {
    {"hue_1", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"saturation_1", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"value_1", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"hue_2", 180.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"saturation_2", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"value_2", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void solid_color_2(led_strip_t *strip, pattern_state_t *state) {
    float h1, s1, v1, h2, s2, v2;
    h1 = state->params[0].value;
    s1 = state->params[1].value / 100.0f; // Scale to 0 - 1
    v1 = state->params[2].value / 100.0f; 
    h2 = state->params[3].value;
    s2 = state->params[4].value / 100.0f; //
    v2 = state->params[5].value / 100.0f; 

    uint8_t r1, g1, b1, r2, g2, b2;
    hsv_to_rgb(h1, s1, v1, &r1, &g1, &b1);
    hsv_to_rgb(h2, s2, v2, &r2, &g2, &b2);
    for (uint16_t i = 0; i < strip->led_count; i++) {
        if (i % 2 == 0) {
            set_led_color(&strip->leds[i], r1, g1, b1);
        } else {
            set_led_color(&strip->leds[i], r2, g2, b2);
        }
        set_led_brightness(&strip->leds[i], 255);
    }
}

// Pattern function parameters
const pattern_param_t rainbow_cycle_params[] = {
    {"pixel_width", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"brightness", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
// ID 0:rainbow cycle pattern
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state) {
    float pixel_width = state->params[0].value / 100.0f;
    float saturation = state->params[1].value / 100.0f;
    uint8_t brightness = state->params[2].value / 100.0f * 255.0f;
    
    for (uint16_t i = 0; i < strip->led_count; i++) {
        float hue = sinf((state->time * state->speed) + ( (float)i * pixel_width * (2.0f * M_PI / strip->led_count))) * 180.0f + 180.0f;

        uint8_t r, g, b;
        hsv_to_rgb(hue, saturation, 1.0f, &r, &g, &b);
        
        set_led_color(&strip->leds[i], r, g, b);
        set_led_brightness(&strip->leds[i], brightness);
    }
}

// ID 1: stroboscope pattern
const pattern_param_t stroboscope_params[] = {
    {"flash_on_off_ratio", 50.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"hue", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"value", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void stroboscope(led_strip_t *strip, pattern_state_t *state){
    float flash_on_off_ratio = state->params[0].value;
    float h, s, v;
    h = state->params[1].value;
    s = state->params[2].value / 100.0f;
    v = state->params[3].value / 100.0f;

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
const pattern_param_t fire_params[] = {
    {"flicker_intensity", 50.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"yellow_level", 0.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void fire(led_strip_t *strip, pattern_state_t *state) {
    float flicker_intensity = state->params[0].value / 100.0f;
    float yellow_level = state->params[1].value / 100.0f;

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

// ID 4: Sparkle pattern
const pattern_param_t sparkle_params[] = {
    {"sparkle_chance", 50.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"sparkle_hue", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"sparkle_saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"sparkle_brightness", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"background_hue", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"background_saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"background_brightness", 50.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void sparkle(led_strip_t *strip, pattern_state_t *state) {
    float sparkle_chance = state->params[0].value / 100.0f;
    float sparkle_hue = state->params[1].value;
    float sparkle_saturation = state->params[2].value / 100.0f;
    float sparkle_brightness = state->params[3].value / 100.0f * 255.0f;
    float background_hue = state->params[4].value;
    float background_saturation = state->params[5].value / 100.0f;
    float background_brightness = state->params[6].value / 100.0f * 255.0f;
    
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

// ID 5: Moving band pattern
const pattern_param_t moving_band_params[] = {
    {"hue", 0.0f, PARAM_MAX_DEGREES, PARAM_TYPE_DEGREES},
    {"saturation", 100.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"band_width", 10.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"band_number", 10.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE},
    {"band_smoothing", 0.0f, PARAM_MAX_PERCENTAGE, PARAM_TYPE_PERCENTAGE}
};
void moving_band(led_strip_t *strip, pattern_state_t *state) {
    float hue = state->params[0].value;
    float saturation = state->params[1].value / 100.0f;
    float band_width = state->params[2].value / 100.0f;
    float band_number = state->params[3].value / 100.0f;
    float band_smoothing = state->params[4].value / 100.0f;

    uint16_t band_count = (uint16_t)(band_number * 10.0f) + 1; // Map to 1-10 bands

    for (uint16_t i = 0; i < strip->led_count; i++) {
        // Calculate position within band
        float movement = state->time * state->speed * 0.5f;
        float spatial_position = (float)i / strip->led_count;
        float position = fmodf(movement + spatial_position * band_count, 1.0f);
        
        // Determine if LED is within the band
        if (position < band_width) {
            // Calculate brightness based on distance from band center with smoothing
            float distance = (position - (band_width / 2.0f)) / (band_width / 2.0f); // -1.0 to +1.0
            distance = fabsf(distance); // 0.0 at center to 1.0 at edges
            if (distance > 1.0f) distance = 1.0f;

            float brightness_factor = 1.0f;
            if (band_smoothing > 0.0f) {
                brightness_factor = 1.0f - powf(distance, 1.0f / (band_smoothing + 0.0001f)); // Smoother falloff 
            }
            uint8_t r, g, b;
            hsv_to_rgb(hue, saturation, 1.0f, &r, &g, &b);
            set_led_color(&strip->leds[i], r, g, b);
            set_led_brightness(&strip->leds[i], (uint8_t)(brightness_factor * 255));
        } else {
            // Set to off or background color
            set_led_brightness(&strip->leds[i], 0);
        }
    }
        
}

// Pattern registry
const pattern_t pattern_registry[] = {
    {"Solid Color", solid_color, (pattern_param_t*)solid_color_params, sizeof(solid_color_params) / sizeof(pattern_param_t)},
    {"Solid Color 2", solid_color_2, (pattern_param_t*)solid_color_params_2, sizeof(solid_color_params_2) / sizeof(pattern_param_t)},
    {"Moving Band", moving_band, (pattern_param_t*)moving_band_params, sizeof(moving_band_params) / sizeof(pattern_param_t)},
    {"Sparkle", sparkle, (pattern_param_t*)sparkle_params, sizeof(sparkle_params) / sizeof(pattern_param_t)},
    {"Fire", fire, (pattern_param_t*)fire_params, sizeof(fire_params) / sizeof(pattern_param_t)},
    {"Rainbow Cycle", rainbow_cycle, (pattern_param_t*)rainbow_cycle_params, sizeof(rainbow_cycle_params) / sizeof(pattern_param_t)},
    {"Stroboscope", stroboscope, (pattern_param_t*)stroboscope_params, sizeof(stroboscope_params) / sizeof(pattern_param_t)},
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


