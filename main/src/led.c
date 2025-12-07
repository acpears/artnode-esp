#include "led.h"

#include <stdint.h>
#include "esp_log.h"

void initialize_led(led_t *led, uint16_t id, uint8_t universe, uint16_t dmx_address) {
    led->id = id;
    led->universe = universe;
    led->dmx_address = dmx_address;
    led->state.brightness = 255; // Default to off
    led->state.color.r = 0;
    led->state.color.g = 0;
    led->state.color.b = 0;
}

void initialize_led_strip(led_strip_t *strip){
    strip->brightness = 255; // Default strip brightness
    
    uint8_t current_universe = strip->start_universe;
    uint16_t current_address = strip->start_address;
    
    for (uint16_t i = 0; i < strip->led_count; i++) {
        // Check if there are enough addresses left in current universe for a complete LED (3 channels)
        if (current_address > DMX_DATA_MAX_LENGTH - 2) { // Need 3 consecutive channels (current + 2 more)
            // Not enough space, move to next universe starting at address 1
            current_universe++;
            current_address = 1;
        }
        
        // Initialize LED with current universe and address
        initialize_led(&strip->leds[i], i + 1, current_universe, current_address);
        
        // Move to next LED position (3 channels forward)
        current_address += 3;
    }
}

void set_led_color(led_t *led, uint8_t r, uint8_t g, uint8_t b) {
    led->state.color.r = r;
    led->state.color.g = g;
    led->state.color.b = b;
}

void set_led_color_from_hex(led_t *led, uint32_t hex_color) {
    // Extract RGB components from hex color
    uint8_t r = (hex_color >> 16) & 0xFF;
    uint8_t g = (hex_color >> 8) & 0xFF;
    uint8_t b = hex_color & 0xFF;

    // Set the LED color
    led->state.color.r = r;
    led->state.color.g = g;
    led->state.color.b = b;
}

void set_led_brightness(led_t *led, uint8_t brightness) {
    led->state.brightness = brightness;
}

void set_dmx_data(led_t *led, uint8_t dmx_data_array[][DMX_DATA_MAX_LENGTH], uint8_t dmx_data_array_length) {

    // Calculate RGB values with brightness scaling
    uint8_t r_value = ((uint16_t)led->state.color.r * led->state.brightness + 127) / 255;
    uint8_t g_value = ((uint16_t)led->state.color.g * led->state.brightness + 127) / 255;
    uint8_t b_value = ((uint16_t)led->state.color.b * led->state.brightness + 127) / 255;
    
    uint8_t channel_values[3] = {r_value, g_value, b_value};

    // Handle each channel individually to support automatic universe overflow
    for (int channel = 0; channel < 3; channel++) {
        // Calculate absolute DMX address for this channel (1-based)
        uint16_t absolute_address = led->dmx_address + channel;

        // Calculate which universe index this address falls into
        // universe_offset 0 for addresses 1-512, universe_offset 1 for addresses 513-1024, etc.
        uint8_t universe_offset = absolute_address / DMX_DATA_MAX_LENGTH; 
        uint8_t universe_index = (led->universe + universe_offset) - 1; // Convert to 0-based for calculation

        // Caclculate the address index within the universe (0-based)
        uint16_t address_index = (absolute_address - 1) % DMX_DATA_MAX_LENGTH; 

        // Set the channel value in the calculated universe and address
        dmx_data_array[universe_index][address_index] = channel_values[channel];
    }
}

void set_strip_dmx_data(led_strip_t *strip, uint8_t dmx_data_array[][DMX_DATA_MAX_LENGTH], uint8_t dmx_data_array_length) {
    for (uint16_t i = 0; i < strip->led_count; i++) {
        strip->leds[i].state.brightness = strip->leds[i].state.brightness * (1.0f * strip->brightness / 255.0f);
        set_dmx_data(&strip->leds[i], dmx_data_array, dmx_data_array_length);
    }
}