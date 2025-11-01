#include "led_config.h"

// Pattern parameters
static float params1[] = {2.0f, 0.1f}; // fire: flicker_intensity=0.1, yellow_level=0.2
static float params2[] = {1.5f, 0.2f}; // fire: flicker_intensity=1.5, yellow_level=0.2
static float params3[] = {2.0f, 0.3f}; // fire: flicker_intensity=2.0, yellow_level=0.3

// LED strip configurations
const led_strip_config_t led_strip_configs[] = {
    {
        .led_count = 100,
        .start_universe = 1,
        .start_address = 1,
        .pattern_id = 0,  // fire pattern
        .speed = 2.0f,
        .pattern_params = params1
    },
    {
        .led_count = 100,
        .start_universe = 2,
        .start_address = 1,
        .pattern_id = 0,  // fire pattern
        .speed = 1.0f,
        .pattern_params = params1
    },
    {
        .led_count = 100,
        .start_universe = 3,
        .start_address = 1,
        .pattern_id = 0,  // fire pattern
        .speed = 1.8f,
        .pattern_params = params1
    },
    {
        .led_count = 100,
        .start_universe = 4,
        .start_address = 1,
        .pattern_id = 0,  // fire pattern
        .speed = 1.8f,
        .pattern_params = params1
    },
    {
        .led_count = 50,
        .start_universe = 1,
        .start_address = 301,
        .pattern_id = 0,  // fire pattern
        .speed = 1.0f,
        .pattern_params = params1
    }
};

const uint8_t led_strip_config_count = sizeof(led_strip_configs) / sizeof(led_strip_config_t);