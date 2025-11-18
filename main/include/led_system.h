#ifndef LED_SYSTEM_H
#define LED_SYSTEM_H

#include "led.h"
#include "patterns.h"

#define MAX_LED_STRIPS 10

// Group controller state
typedef struct {
    float brightness;
    float speed;
    float params[10]; // Up to 10 custom parameters per group
    bool changed;
} group_state_t;

// Controller state
typedef struct {
    group_state_t groups[MAX_LED_STRIPS];
} controller_state_t;

typedef struct {
    led_strip_t* groups; // Array of LED strips
    uint8_t group_count; // Number of LED strips
    controller_state_t* controller_state; // Pointer to controller state
    pattern_state_t* pattern_states; // Array of pattern states
    float master_brightness; // 0.0 - 1.0
    float master_speed; // 0.0 - 1.0
    bool initialized;
} led_system_t;

void init_led_system(led_system_t* led_system);
void cleanup_led_system(led_system_t* led_system);
void update_pattern_state_pattern_id(led_system_t* led_system, uint8_t group_index, uint16_t pattern_id);
void update_led_system_patterns(led_system_t* led_system, float delta_time);
void set_led_system_dmx_data(led_system_t* led_system, uint8_t dmx_data[][512], uint16_t universes);

// Getters
controller_state_t* get_led_system_controller_state(led_system_t* led_system);

#endif // LED_SYSTEM_H