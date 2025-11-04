#include <string.h>
#include <sys/param.h>
#include <math.h>

#include "network.h"
#include "http.h"
#include "artnet.h"
#include "led_system.h"

#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#define HOST_IP_ADDR "192.168.0.21"

#define UNIVERSE_COUNT 4
#define ADDRESSES_PER_UNIVERSE 512

static const char *LOG_TAG = "artnet-client";

// Network status
static network_status_t network_status = {0};
static wifi_ap_status_t wifi_status = {0};

// Dmx data buffer
static uint8_t dmx_data[UNIVERSE_COUNT][ADDRESSES_PER_UNIVERSE] = {0};

// Led system
static led_system_t led_system = {0};

// HTTP controller state
static http_controller_state_t controller_state_global = {0};


// Cleanup function
static void cleanup(){
    cleanup_led_system(&led_system);
    artnet_close();
}

static void artnet_controller_task(void *pvParameters)
{
    while (1) {
        // Wait until network is ready
        while(network_status.network_ready == false) {
            ESP_LOGI(LOG_TAG, "Waiting for network to be ready...");
            vTaskDelay(pdMS_TO_TICKS(1000));

        }

        while(artnet_init(HOST_IP_ADDR) < 0) {
            ESP_LOGE(LOG_TAG, "Failed to initialize Art-Net");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Initialize LED system
        init_led_system(&led_system, &controller_state_global);

        uint32_t last_time = xTaskGetTickCount();
        while (1) {
            // Time management
            uint32_t current_time = xTaskGetTickCount();
            float delta_time = (current_time - last_time) * portTICK_PERIOD_MS / 1000.0f;
            last_time = current_time;

            // Update LED patterns
            update_led_system_patterns(&led_system, delta_time);

            // Prepare DMX data
            set_led_system_dmx_data(&led_system, dmx_data, UNIVERSE_COUNT);

            // Send data
            int err = artnet_send_dmx_array(dmx_data, UNIVERSE_COUNT);
            if (err < 0) {
                ESP_LOGE(LOG_TAG, "Error sending DMX data");
                break;
            }
            
            vTaskDelay(pdMS_TO_TICKS(17)); // ~60 FPS
        }
        // Cleanup
        cleanup_led_system(&led_system);
        artnet_close();
    }
    vTaskDelete(NULL);
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_wifi(&wifi_status);
    init_ethernet_static_ip("192.168.0.2", "192.168.0.1", "255.255.255.0", false, &network_status);
    init_http_server(&wifi_status, &controller_state_global);

    xTaskCreate(artnet_controller_task, "artnet_controller_task", 16384, NULL, 5, NULL);
}
