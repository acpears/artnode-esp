#include "led_system.h"
#include "led_config.h"

#include <stdlib.h>
#include <stddef.h>
#include "esp_log.h"

#define LOG_TAG "led_system"

void init_led_system(led_system_t* led_system, http_controller_state_t* controller_state) {
    if (led_system->initialized) {
        return; // Already initialized
    }

    led_system->strip_count = led_strip_config_count;
    ESP_LOGI(LOG_TAG, "Initializing LED system with %d strips", led_system->strip_count);
    for(uint8_t i = 0; i < led_system->strip_count; i++) {
        const led_strip_config_t* config = &led_strip_configs[i];
        led_strip_t* strip = led_system->strips + i;

        strip->leds = malloc(sizeof(led_t) * config->led_count);
        strip->led_count = config->led_count;
        strip->start_universe = config->start_universe;
        strip->start_address = config->start_address;
        initialize_led_strip(strip);

        // Get pattern function by ID and initialize pattern state
        pattern_func_t pattern_func = get_pattern_function(config->pattern_id);
        if (pattern_func == NULL) {
            ESP_LOGE(LOG_TAG, "Skipping invalid strip %d. Invalid pattern ID %d", i, config->pattern_id);
            continue;
        }
        init_pattern_state(&led_system->pattern_states[i], config->pattern_id, config->speed, config->pattern_params, controller_state);
    }
    led_system->initialized = true;
}

void cleanup_led_system(led_system_t* led_system) {
    ESP_LOGI(LOG_TAG, "Cleaning up LED system");
    if (!led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Cleanup skipped: LED system not initialized");
        return; // Not initialized
    }

    for(uint8_t i = 0; i < led_system->strip_count; i++) {
        free(led_system->strips[i].leds);
        led_system->strips[i].leds = NULL;
    }
    led_system->initialized = false;
}

void update_led_system_patterns(led_system_t* led_system, float delta_time) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Update patterns skipped: LED system not initialized");
        return;
    }
    for(uint8_t i = 0; i < led_system->strip_count; i++) {
        update_pattern(&led_system->pattern_states[i], &led_system->strips[i], delta_time);
    }
}

void set_led_system_dmx_data(led_system_t* led_system, uint8_t dmx_data[][512], uint16_t universes) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Set DMX data skipped: LED system not initialized");
        return;
    }
    for(uint8_t i = 0; i < led_system->strip_count; i++){
        set_strip_dmx_data(&led_system->strips[i], dmx_data, universes);
    }
}