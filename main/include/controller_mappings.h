#ifndef CONTROLLER_MAPPINGS_H
#define CONTROLLER_MAPPINGS_H

#include "patterns.h"
#include "controller_server.h"

#include <stdint.h>

void map_group_state_to_pattern_params(group_state_t *group_state, pattern_param_t *pattern_params, uint8_t param_count);
void map_group_master_to_strip_brightness(group_state_t *group_state, led_strip_t *strip);
void map_group_speed_to_pattern_speed(group_state_t *group_state, pattern_state_t *pattern_state);
void update_from_controller_group_state(led_strip_t *strip, group_state_t *group_state, pattern_state_t *pattern_state) ;
#endif // CONTROLLER_MAPPINGS_H