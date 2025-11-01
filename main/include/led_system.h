#ifndef LED_SYSTEM_H
#define LED_SYSTEM_H

#include "led.h"
#include "patterns.h"

typedef struct {
    led_strip_t strips[10];
    pattern_state_t pattern_states[10];
    uint8_t strip_count;
    bool initialized;
} led_system_t;

void init_led_system(led_system_t* led_system);
void cleanup_led_system(led_system_t* led_system);
void update_led_system_patterns(led_system_t* led_system, float delta_time);
void set_led_system_dmx_data(led_system_t* led_system, uint8_t dmx_data[][512], uint16_t universes);

#endif // LED_SYSTEM_H