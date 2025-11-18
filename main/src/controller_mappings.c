#include "controller_mappings.h"
#include "esp_log.h"

#define LOG_TAG "Controller Mappings"

void map_group_state_to_pattern_params(group_state_t *group_state, pattern_param_t *pattern_params, uint8_t param_count) {
    if (group_state == NULL || pattern_params == NULL) {
        ESP_LOGE(LOG_TAG, "Invalid controller state or pattern parameters");
        return;
    }

    for (uint8_t i = 0; i < param_count; i++) {
        pattern_params[i].value = group_state->params[i];
    }
}

void map_group_master_to_strip_brightness(group_state_t *group_state, led_strip_t *strip) {
    if (group_state == NULL || strip == NULL) {
        ESP_LOGE(LOG_TAG, "Invalid controller state or LED strip");
        return;
    }

    strip->brightness = (uint8_t)(group_state->brightness * 255.0f);
}

void map_group_speed_to_pattern_speed(group_state_t *group_state, pattern_state_t *pattern_state) {
    if (group_state == NULL || pattern_state == NULL) {
        ESP_LOGE(LOG_TAG, "Invalid controller state or pattern state");
        return;
    }

    pattern_state->speed = group_state->speed * 4.0f; // Scale speed for patterns
}

void update_from_controller_group_state(led_strip_t *strip, group_state_t *group_state, pattern_state_t *pattern_state) {
    if (group_state == NULL || pattern_state == NULL) {
        ESP_LOGE(LOG_TAG, "Invalid controller state or pattern state");
        return;
    }

    map_group_state_to_pattern_params(group_state, pattern_state->params, pattern_state->param_count);
    map_group_master_to_strip_brightness(group_state, strip);
    map_group_speed_to_pattern_speed(group_state, pattern_state);
}