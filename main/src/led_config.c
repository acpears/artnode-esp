#include "led_config.h"

// LED strip configurations
const led_strip_config_t led_strip_configs[] = {
    {
        .name = "Strip 1",
        .led_count = 25,
        .start_universe = 1,
        .start_address = 1,
        .pattern_id = 0, 
        .speed = 0.5f,
        .brightness = 1.0f
    },
    {
        .name = "Strip 2",
        .led_count = 25,
        .start_universe = 1,
        .start_address = 76,
        .pattern_id = 1, 
        .speed = 0.5f,
        .brightness = 1.0f
    },

};

const uint8_t led_strip_config_count = sizeof(led_strip_configs) / sizeof(led_strip_config_t);