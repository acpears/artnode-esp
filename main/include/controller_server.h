#ifndef CONTROLLER_SERVER_H
#define CONTROLLER_SERVER_H
#include "network.h"
#include "led_system.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_http_server.h"
#include "esp_err.h"

// Public API
void init_http_server(wifi_ap_status_t* wifi_ap_status, led_system_t* led_system);

#endif // CONTROLLER_SERVER_H