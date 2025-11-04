#ifndef PATTERNS_H
#define PATTERNS_H

#include <stdint.h>
#include <stdbool.h>

#include "led.h"
#include "http.h"

typedef struct {
    uint16_t pattern_id;
    void *pattern_data;      // Pattern-specific data (optional)
    http_controller_state_t *controller_state; // Pointer to controller state
    float time;              // Internal time counter
    float speed;             // Speed multiplier
    bool active;             // Whether pattern is running
} pattern_state_t;

// Pattern function type
typedef void (*pattern_func_t)(led_strip_t *strip, pattern_state_t *state);

// Pattern management
void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float speed, void *pattern_data, http_controller_state_t *controller_state);
void update_pattern(pattern_state_t *state, led_strip_t *strip, float delta_time);
pattern_func_t get_pattern_function(uint16_t pattern_id);

// Pattern functions
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state);
void stroboscope(led_strip_t *strip, pattern_state_t *state);
void fire(led_strip_t *strip, pattern_state_t *state);
void solid_color(led_strip_t *strip, pattern_state_t *state);
void sparkle(led_strip_t *strip, pattern_state_t *state);

// Pattern registry for easy lookup
extern const pattern_func_t pattern_functions[];
extern const char* pattern_names[];
extern const uint8_t pattern_count;

#endif // PATTERNS_H