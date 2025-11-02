#include "led_config.h"

// Pattern parameters
static float params1[] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.01f};

// LED strip configurations
const led_strip_config_t led_strip_configs[] = {
    {
        .led_count = 50,
        .start_universe = 1,
        .start_address = 1,
        .pattern_id = 4, 
        .speed = 1.0f,
        .pattern_params = params1
    }
};

const uint8_t led_strip_config_count = sizeof(led_strip_configs) / sizeof(led_strip_config_t);