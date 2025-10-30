#include <string.h>
#include <sys/param.h>
#include <math.h>

#include "artnet.h"
#include "handlers.h"
#include "network.h"
#include "led.h"
#include "colours.h"
#include "patterns.h"

#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"

#define HOST_IP_ADDR "192.168.0.21"

#define UNIVERSE_COUNT 4
#define ADDRESSES_PER_UNIVERSE 512

static const char *TAG = "artnet-client";

// Flag to indicate if IP address is acquired
static bool got_ip = false;
// Flag to indicate if ethernet link is up
static bool eth_link_up = false;

static uint8_t dmx_data[UNIVERSE_COUNT][ADDRESSES_PER_UNIVERSE] = {0};
static pattern_state_t pattern_states[4];

static void artnet_controller_task(void *pvParameters)
{
    while (1) {
        while (!eth_link_up || !got_ip) {
            ESP_LOGI(TAG, "Waiting for network connection to be setupt...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if(artnet_init(HOST_IP_ADDR) < 0) {
            ESP_LOGE(TAG, "Failed to initialize Art-Net client");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for 2 seconds before sending DMX data

        led_strip_t strip_1 = {
            // Allocate memory for LEDs
            .leds = malloc(sizeof(led_t) * 150),
            .led_count = 150,
            .start_universe = 1,
            .start_address = 1
        };

        led_strip_t strip_2 = {
            // Allocate memory for LEDs
            .leds = malloc(sizeof(led_t) * 150),
            .led_count = 150,
            .start_universe = 2,
            .start_address = 1
        };

        led_strip_t strip_3 = {
            // Allocate memory for LEDs
            .leds = malloc(sizeof(led_t) * 150),
            .led_count = 150,
            .start_universe = 3,
            .start_address = 1
        };


        led_strip_t strip_4 = {
            // Allocate memory for LEDs
            .leds = malloc(sizeof(led_t) * 150),
            .led_count = 150,
            .start_universe = 4,
            .start_address = 1
        };


        initialize_led_strip(&strip_1);
        initialize_led_strip(&strip_2);
        initialize_led_strip(&strip_3);
        initialize_led_strip(&strip_4);

        float params_1[] = {3.0f}; // Full hue cycle
        float params_2[] = {0.0f, 1.0f, 1.0f, 0.1f}; // Full hue cycle


        init_pattern_state(&pattern_states[0], 0, 1.0f, (void *)params_1); // Pattern state for strip 1
        init_pattern_state(&pattern_states[1], 1, 1.0f, (void *)params_1); // Pattern state for strip 2
        init_pattern_state(&pattern_states[2], 1, 1.0f, (void *)params_1); // Pattern state for strip 3
        init_pattern_state(&pattern_states[3], 1, 1.0f, (void *)params_1); // Pattern state for strip 4


        uint32_t last_time = xTaskGetTickCount();
        while (1) {
            uint32_t current_time = xTaskGetTickCount();
            float delta_time = (current_time - last_time) * portTICK_PERIOD_MS / 1000.0f;
            last_time = current_time;
            
            // Update all patterns
            update_pattern(&pattern_states[0], fire, &strip_1, delta_time);
            update_pattern(&pattern_states[1], fire, &strip_2, delta_time);
            update_pattern(&pattern_states[2], fire, &strip_3, delta_time);
            update_pattern(&pattern_states[3], fire, &strip_4, delta_time);

            set_strip_dmx_data(&strip_1, dmx_data, UNIVERSE_COUNT);
            set_strip_dmx_data(&strip_2, dmx_data, UNIVERSE_COUNT);
            set_strip_dmx_data(&strip_3, dmx_data, UNIVERSE_COUNT);
            set_strip_dmx_data(&strip_4, dmx_data, UNIVERSE_COUNT);

            // Send data
            int err = artnet_send_dmx(1, dmx_data[0], ADDRESSES_PER_UNIVERSE);
            int err2 = artnet_send_dmx(2, dmx_data[1], ADDRESSES_PER_UNIVERSE);
            int err3 = artnet_send_dmx(3, dmx_data[2], ADDRESSES_PER_UNIVERSE);
            int err4 = artnet_send_dmx(4, dmx_data[3], ADDRESSES_PER_UNIVERSE);
            if (err < 0 || err2 < 0 || err3 < 0 || err4 < 0) {
                ESP_LOGE(TAG, "Error sending DMX data");
                break;
            }
            
            vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS
        }
        free(strip_1.leds);
        free(strip_2.leds);
        free(strip_3.leds);
        free(strip_4.leds);
        artnet_close();
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    init_network_static_ip("192.168.0.2", "192.168.0.1", "255.255.255.0", false, &got_ip, &eth_link_up);

    xTaskCreate(artnet_controller_task, "artnet_controller_task", 16384, NULL, 5, NULL);
}
