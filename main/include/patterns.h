#ifndef PATTERNS_H
#define PATTERNS_H

#include <stdint.h>
#include <stdbool.h>

#include "led.h"

#define MAX_PATTERN_PARAMS 20

// Pattern parameter descriptor
typedef struct {
    char name[32];          // Parameter name (e.g., "hue", "speed", "intensity")
    float value;            // Current value
    const float min_value;        // Minimum allowed value
    const float max_value;        // Maximum allowed value
    const float default_value;    // Default value
} pattern_param_t;

// Pattern state
typedef struct {
    uint16_t pattern_id;
    pattern_param_t params[MAX_PATTERN_PARAMS];  // Copy of parameters for this instance
    uint8_t param_count;     // Number of parameters used
    float time;              // Internal time counter
    float speed;             // Speed multiplier
    float brightness;       // Brightness level
    bool active;             // Whether pattern is running
} pattern_state_t;

// Pattern function type
typedef void (*pattern_func_t)(led_strip_t *strip, pattern_state_t *state);

typedef struct {
    char name[32];
    pattern_func_t func;
    pattern_param_t *params;
    uint8_t param_count;
} pattern_t;

// Pattern management
void update_pattern(pattern_state_t *state, led_strip_t *strip, float delta_time);

// Pattern functions
void rainbow_cycle(led_strip_t *strip, pattern_state_t *state);
void stroboscope(led_strip_t *strip, pattern_state_t *state);
void fire(led_strip_t *strip, pattern_state_t *state);
void solid_color(led_strip_t *strip, pattern_state_t *state);
void sparkle(led_strip_t *strip, pattern_state_t *state);

// Helpers
pattern_func_t get_pattern_function(uint8_t pattern_id);
pattern_param_t* get_pattern_params(uint8_t pattern_id);
uint8_t get_pattern_param_count(uint8_t pattern_id);
char* get_pattern_name(uint8_t pattern_id);
uint8_t get_total_pattern_count();

#endif // PATTERNS_H