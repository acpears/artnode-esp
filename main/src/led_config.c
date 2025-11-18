#include "led_config.h"

// LED strip configurations
const led_strip_config_t led_strip_configs[] = {
    {
        .name = "Strip 1",
        .led_count = 50,
        .start_universe = 1,
        .start_address = 1,
        .pattern_id = 5, 
        .speed = 0.5f,
        .brightness = 1.0f
    },
    {
        .name = "Strip 2",
        .led_count = 50,
        .start_universe = 2,
        .start_address = 1,
        .pattern_id = 5, 
        .speed = 0.5f,
        .brightness = 1.0f
    },
    {
        .name = "Strip 3",
        .led_count = 50,
        .start_universe = 3,
        .start_address = 1,
        .pattern_id = 5, 
        .speed = 0.5f,
        .brightness = 1.0f
    },
    {
        .name = "Strip 4",
        .led_count = 50,
        .start_universe = 4,
        .start_address = 1,
        .pattern_id = 5, 
        .speed = 0.5f,
        .brightness = 1.0f
    }

};

const uint8_t led_strip_config_count = sizeof(led_strip_configs) / sizeof(led_strip_config_t);