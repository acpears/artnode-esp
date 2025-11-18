#include "led_system.h"
#include "led_config.h"
#include "controller_mappings.h"

#include <stdlib.h>
#include <stddef.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#define LOG_TAG "led_system"
#define STORAGE_GROUP_NAMESPACE "group_state"
#define STORAGE_PATTERN_NAMESPACE "pattern_state"

// forward declaration
static void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float brightness, float speed);
static void init_group_state(group_state_t* group_state, uint8_t pattern_id, float brightness, float speed);
static esp_err_t store_group_state_to_nvs(group_state_t* group_state, uint8_t group_index, uint8_t pattern_id);
static esp_err_t load_group_state_from_nvs(group_state_t* group_state, uint8_t group_index, uint8_t pattern_id);
static void format_group_state_nvs_key(char* key_buffer, size_t buffer_size, uint8_t group_index, uint8_t pattern_id);
static void store_pattern_id_to_nvs(pattern_state_t* pattern_state, uint8_t pattern_state_index);
static void load_pattern_id_from_nvs(pattern_state_t* pattern_state, uint8_t pattern_state_index);


// Method that takes the led configurations and initializes the led system
void init_led_system(led_system_t* led_system) {
    if (led_system->initialized) {
        return; // Already initialized
    }

    // Allocate memory for LED groups, group states, and pattern states
    led_system->group_count = led_strip_config_count;
    ESP_LOGI(LOG_TAG, "Initializing LED system with %d groups", led_system->group_count);
    led_system->groups = malloc(sizeof(led_strip_t) * led_system->group_count);
    led_system->pattern_states = malloc(sizeof(pattern_state_t) * led_system->group_count);
    led_system->controller_state = malloc(sizeof(controller_state_t));

    // Iterate over each LED group configuration from led_config.c
    for(uint8_t i = 0; i < led_system->group_count; i++) {
        const led_strip_config_t* config = &led_strip_configs[i];
        led_strip_t* group = led_system->groups + i;

        // Setup and initialize group 
        group->leds = malloc(sizeof(led_t) * config->led_count);
        group->led_count = config->led_count;
        group->start_universe = config->start_universe;
        group->start_address = config->start_address;
        initialize_led_strip(group);

        // Initialize pattern state
        init_pattern_state(&led_system->pattern_states[i], config->pattern_id, config->brightness, config->speed);
        // Try and load stored pattern state pattern id from NVS 
        load_pattern_id_from_nvs(&led_system->pattern_states[i], i);

        // Load stored group state from NVS else initialize with defaults
        esp_err_t err = load_group_state_from_nvs(&led_system->controller_state->groups[i], i, led_system->pattern_states[i].pattern_id);
        if (err == ESP_OK) {
            ESP_LOGW(LOG_TAG, "Loaded stored state for group %d from NVS", i);
            update_from_controller_group_state(&led_system->groups[i], &led_system->controller_state->groups[i], &led_system->pattern_states[i]);
        } else {
            ESP_LOGW(LOG_TAG, "Using default state for group %d", i);
            init_group_state(&led_system->controller_state->groups[i], led_system->pattern_states[i].pattern_id, led_system->pattern_states[i].brightness, led_system->pattern_states[i].speed);
        }
    }
    led_system->initialized = true;
}

// Initialize pattern state
static void init_pattern_state(pattern_state_t *state, uint16_t pattern_id, float brightness, float speed) {
    state->pattern_id = pattern_id;
    
    // Get the pattern's default parameters
    pattern_param_t* default_params = get_pattern_params(pattern_id);
    state->param_count = get_pattern_param_count(pattern_id);
    
    // Copy the parameters so this pattern state has its own copy
    for (uint8_t i = 0; i < state->param_count; i++) {
        memcpy(&state->params[i], &default_params[i], sizeof(pattern_param_t));
    }
    
    state->time = 0.0f; // Initialize time to zero
    state->speed = speed;
    state->brightness = brightness;
    state->active = true;
}

// Initialize group state with default values from configuration and pattern parameters
static void init_group_state(group_state_t* group_state, uint8_t pattern_id, float brightness, float speed) {
    group_state->brightness = brightness;
    group_state->speed = speed;
    uint8_t param_count = get_pattern_param_count(pattern_id);
    pattern_param_t* params = get_pattern_params(pattern_id);   
    for (uint8_t i = 0; i < param_count; i++) {
        group_state->params[i] = params[i].value;
    }
}

// Cleanup LED system resources
void cleanup_led_system(led_system_t* led_system) {
    ESP_LOGI(LOG_TAG, "Cleaning up LED system");
    if (!led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Cleanup skipped: LED system not initialized");
        return; // Not initialized
    }

    for(uint8_t i = 0; i < led_system->group_count; i++) {
        free(led_system->groups[i].leds);
        led_system->groups[i].leds = NULL;
    }
    free(led_system->groups);
    free(led_system->pattern_states);
    free(led_system->controller_state);
    led_system->groups = NULL;
    led_system->pattern_states = NULL;
    led_system->controller_state = NULL;
    led_system->initialized = false;
}


// Method that will be called periodically to update all patterns in the LED system with the given delta time
void update_led_system_patterns(led_system_t* led_system, float delta_time) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Update patterns skipped: LED system not initialized");
        return;
    }
    for(uint8_t i = 0; i < led_system->group_count; i++) {
        group_state_t* group_state = &led_system->controller_state->groups[i];

        if(group_state->changed) {
            update_from_controller_group_state(&led_system->groups[i], group_state, &led_system->pattern_states[i]);
            group_state->changed = false;
            store_group_state_to_nvs(group_state, i, led_system->pattern_states[i].pattern_id);
        }

        update_pattern(&led_system->pattern_states[i], &led_system->groups[i], delta_time);
    }
}

// Method to set DMX data for all LED strips in the system
void set_led_system_dmx_data(led_system_t* led_system, uint8_t dmx_data_array[][512], uint8_t dmx_data_array_length) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Set DMX data skipped: LED system not initialized");
        return;
    }
    for(uint8_t i = 0; i < led_system->group_count; i++){
        set_strip_dmx_data(&led_system->groups[i], dmx_data_array, dmx_data_array_length);
    }
}

// Method to update the pattern id of a given group index
void update_pattern_state_pattern_id(led_system_t* led_system, uint8_t group_index, uint16_t pattern_id) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Update pattern state skipped: LED system not initialized");
        return;
    }
    if (group_index >= led_system->group_count) {
        ESP_LOGE(LOG_TAG, "Invalid group index %d for updating pattern state", group_index);
        return;
    }

    // Pattern state to update
    pattern_state_t* pattern_state = &led_system->pattern_states[group_index];

    // Reset and initialize pattern state
    init_pattern_state(pattern_state, pattern_id,  1.0f, 0.5f);

    // Store updated pattern id to NVS for retrieval on next startup
    store_pattern_id_to_nvs(pattern_state, group_index);
        
    // Try and load a stored group state from NVS else initialize with defaults
    group_state_t *group_state = &led_system->controller_state->groups[group_index];
    esp_err_t err = load_group_state_from_nvs(group_state, group_index, pattern_id);
    if (err != ESP_OK) {
        ESP_LOGW(LOG_TAG, "Using default state for group %d", group_index);
        init_group_state(group_state, pattern_id, 1.0f, 0.5f);
    } else {
        ESP_LOGI(LOG_TAG, "Using stored state for group %d", group_index);
        update_from_controller_group_state(&led_system->groups[group_index], group_state, pattern_state);
    }

}

controller_state_t* get_led_system_controller_state(led_system_t* led_system) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Get controller state skipped: LED system not initialized");
        return NULL;
    }
    return led_system->controller_state;
}

// Store controller group state into NVS for given pattern id
static esp_err_t store_group_state_to_nvs(group_state_t* group_state, uint8_t group_index, uint8_t pattern_id) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_GROUP_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_storage_size = sizeof(group_state_t);
    char key[32];
    format_group_state_nvs_key(key, sizeof(key), group_index, pattern_id);

    err = nvs_set_blob(nvs_handle, key, group_state, required_storage_size);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error setting group state blob: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(LOG_TAG, "Group state stored to NVS successfully for pattern %d in group %d", pattern_id, group_index);
    return ESP_OK;
}

// Load group state from NVS
static esp_err_t load_group_state_from_nvs(group_state_t* group_state, uint8_t group_index, uint8_t pattern_id) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_GROUP_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_storage_size = sizeof(group_state_t);
    char key[16];
    format_group_state_nvs_key(key, sizeof(key), group_index, pattern_id);

    err = nvs_get_blob(nvs_handle, key, group_state, &required_storage_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(LOG_TAG, "Nothing store for key %s", key);
    } else if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error getting blob for key %s: %s", key, esp_err_to_name(err));
    } else {
        ESP_LOGI(LOG_TAG, "Loaded key %s from NVS", key);
    }
    nvs_close(nvs_handle);
    return err;
}

// Format NVS key for storing/loading group state as follows: "group_<group_index>_pattern_<pattern_id>"
static void format_group_state_nvs_key(char* key_buffer, size_t buffer_size, uint8_t group_index, uint8_t pattern_id) {
    snprintf(key_buffer, buffer_size, "g_%d_p_%d", group_index, pattern_id);
}

// Store pattern state to NVS
static void store_pattern_id_to_nvs(pattern_state_t* pattern_state, uint8_t pattern_state_index) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    uint8_t pattern_id = pattern_state->pattern_id;

    err = nvs_open(STORAGE_PATTERN_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }

    size_t required_storage_size = sizeof(pattern_state_t);
    char key[16];
    snprintf(key, sizeof(key), "pid_%d", pattern_state_index);
    err = nvs_set_blob(nvs_handle, key, pattern_state, required_storage_size);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error setting blob: %s", esp_err_to_name(err));
        return;
    }
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(LOG_TAG, "Pattern id %d stored to NVS successfully for pattern state %d", pattern_id, pattern_state_index);
    return;
}

// Load pattern state from NVS
static void load_pattern_id_from_nvs(pattern_state_t* pattern_state, uint8_t pattern_state_index) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_PATTERN_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }   
    size_t required_storage_size = sizeof(pattern_state_t);
    char key[16];
    snprintf(key, sizeof(key), "pid_%d", pattern_state_index); 
    err = nvs_get_blob(nvs_handle, key, pattern_state, &required_storage_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(LOG_TAG, "Nothing store for key %s", key);
    } else if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Error getting blob for key %s: %s", key, esp_err_to_name(err));
    } else {
        ESP_LOGI(LOG_TAG, "Loaded key %s from NVS", key);
    }
    nvs_close(nvs_handle);
    return;
}

// Get the number of universes used by the LED system
uint8_t get_led_system_universe_count(led_system_t* led_system) {
    if (!led_system || !led_system->initialized) {
        ESP_LOGE(LOG_TAG, "Get universe count skipped: LED system not initialized");
        return 0;
    }
    uint8_t max_universe = 0;
    for (uint8_t i = 0; i < led_system->group_count; i++) {
        led_strip_t* strip = &led_system->groups[i];
        uint8_t strip_end_universe = strip->start_universe + ((strip->start_address + (strip->led_count * 3) - 1) / DMX_DATA_MAX_LENGTH);
        if (strip_end_universe > max_universe) {
            max_universe = strip_end_universe;
        }
    }
    return max_universe;
}