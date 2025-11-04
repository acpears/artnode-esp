#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stdint.h>

#include "network.h"

#include "esp_http_server.h"
#include "esp_err.h"

typedef struct {
    bool row[10][10]; // 10x10 grid of buttons
    uint8_t slider[10]; // int values 0-255 corresponding to slider positions
} http_controller_state_t;

// Public API
void init_http_server(wifi_ap_status_t* wifi_ap_statu, http_controller_state_t* controller_state);

#endif // HTTP_H