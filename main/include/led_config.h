#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <stdint.h>

typedef struct {
    // strip configuration
    char name[32];
    uint16_t led_count;
    uint16_t start_universe;
    uint16_t start_address;

    // default pattern configuration
    uint8_t pattern_id;
    float speed;
    float brightness;
} led_strip_config_t;

// External declarations (defined in led_config.c)
extern const led_strip_config_t led_strip_configs[];
extern const uint8_t led_strip_config_count;

#endif // LED_CONFIG_H