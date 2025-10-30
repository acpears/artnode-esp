#ifndef PATTERNS_H
#define PATTERNS_H

#include "led.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t pattern_id;
    float time;              // Internal time counter
    float speed;             // Speed multiplier
    bool active;             // Whether pattern is running
    void *pattern_data;      // Pattern-specific data (optional)
} pattern_state_t;

// Pattern function type
typedef void (*pattern_func_t)(led_strip_t *strip, pattern_state_t *state);

// Pattern management
void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float speed, void *pattern_data);
void update_pattern(pattern_state_t *state, pattern_func_t pattern_func, 
                   led_strip_t *strip, float delta_time);

// Pattern functions
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state);
void stroboscope(led_strip_t *strip, pattern_state_t *state);
void fire(led_strip_t *strip, pattern_state_t *state);

#endif // PATTERNS_H