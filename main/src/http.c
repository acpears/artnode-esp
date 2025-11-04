#include <stdlib.h>
#include <string.h>

#include "http.h"

#include "esp_log.h"
#include "esp_http_server.h"

#define LOG_TAG "http"

static http_controller_state_t* controller_state_global = NULL;

// Forward declarations for static functions
static esp_err_t post_button_handler(httpd_req_t *req);
static esp_err_t post_slider_handler(httpd_req_t *req);
static esp_err_t get_url_query_param(httpd_req_t *req, const char *param_name, char *param_value, size_t param_value_size);
static httpd_handle_t start_webserver();
static void init_handlers(httpd_handle_t *server);

static const httpd_uri_t button_uri = {
    .uri       = "/button",               // the address at which the resource can be found
    .method    = HTTP_POST,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = post_button_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

static const httpd_uri_t slider_uri = {
    .uri       = "/slider",               // the address at which the resource can be found
    .method    = HTTP_POST,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = post_slider_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

// Initialize URI handlers
static void init_handlers(httpd_handle_t *server) {
    httpd_register_uri_handler(*server, &button_uri);
    httpd_register_uri_handler(*server, &slider_uri);
}

// Helper function: get URL query parameter (e.g., /api/endpoint?param=value)
static esp_err_t get_url_query_param(httpd_req_t *req, const char *param_name, char *param_value, size_t param_value_size)
{
    char *buf;
    size_t buf_len;

    // Get URL query string length and allocate memory for length + 1
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (buf == NULL) {
            return ESP_ERR_NO_MEM;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            // Get value of expected key from query string
            if (httpd_query_key_value(buf, param_name, param_value, param_value_size) == ESP_OK) {
                free(buf);
                return ESP_OK;
            }
        }
        free(buf);
    }
    return ESP_FAIL;
}

static httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(LOG_TAG, "HTTP server started");
        return server;
    } else {
        ESP_LOGE(LOG_TAG, "Failed to start HTTP server");
        return NULL;
    }
}

void init_http_server(wifi_ap_status_t* wifi_ap_status, http_controller_state_t* controller_state)
{
    controller_state_global = controller_state;

    while(!wifi_ap_status->network_ready) {
        ESP_LOGW(LOG_TAG, "WiFi AP not started, HTTP server may not be reachable");
        vTaskDelay(pdMS_TO_TICKS(2000));   
    }

    httpd_handle_t server = start_webserver();
    if (server) {
        init_handlers(&server);
    }
}

// URI HANDLERS
// Handle POST /button?row=X&col=Y
static esp_err_t post_button_handler(httpd_req_t *req)
{
    const char *param_name_row = "row";
    const char *param_name_col = "col";
    char param_value_row[16];
    char param_value_col[16];

    if (get_url_query_param(req, param_name_row, param_value_row, sizeof(param_value_row)) == ESP_OK &&
        get_url_query_param(req, param_name_col, param_value_col, sizeof(param_value_col)) == ESP_OK) {
        // Process the parameters
        ESP_LOGI(LOG_TAG, "Row: %s, Col: %s", param_value_row, param_value_col);
        uint8_t row = atoi(param_value_row);
        uint8_t col = atoi(param_value_col);
        if(row == 0 || col == 0 || row > 10 || col > 10) {
            ESP_LOGW(LOG_TAG, "Row or Column invalid values");
            httpd_resp_set_status(req, "400 Bad Request");
            const char* error_response = "Row and Column are invalid (must be 1-10)";
            httpd_resp_send(req, error_response, strlen(error_response));
            return ESP_OK;
        }
        row -= 1; // Convert to 0-based index
        col -= 1; // Convert to 0-based index

        if (controller_state_global) {
            memset(controller_state_global->row[row], 0, sizeof(controller_state_global->row[row])); // Reset all columns in the row
            controller_state_global->row[row][col] = true;
        }
        httpd_resp_send(req, NULL, 0);
    } else {
        ESP_LOGE(LOG_TAG, "Failed to get query parameters");

        // Send error response
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Missing required parameters: row and col";
        httpd_resp_send(req, error_response, strlen(error_response));
    }

    return ESP_OK;
}

// Handle POST /slider?slider=N&value=V
static esp_err_t post_slider_handler(httpd_req_t *req)
{
    const char *param_name_slider = "slider";
    const char *param_name_value = "value";
    char param_value_slider[16];
    char param_value_value[16];

    if (get_url_query_param(req, param_name_slider, param_value_slider, sizeof(param_value_slider)) == ESP_OK &&
        get_url_query_param(req, param_name_value, param_value_value, sizeof(param_value_value)) == ESP_OK) {
        // Process the parameters
        ESP_LOGI(LOG_TAG, "Slider: %s, Value: %s", param_value_slider, param_value_value);
        uint8_t slider = atoi(param_value_slider);
        uint8_t value = atoi(param_value_value);

        if(slider == 0 || slider > 10 || value > 255) {
            ESP_LOGW(LOG_TAG, "Slider index invalid value");
            httpd_resp_set_status(req, "400 Bad Request");
            const char* error_response = "Slider index is invalid (must be 1-10) and value (must be 0-255)";
            httpd_resp_send(req, error_response, strlen(error_response));
            return ESP_OK;
        }

        slider -= 1; // Convert to 0-based index
        if (controller_state_global) {
            controller_state_global->slider[slider] = value;
        }
        httpd_resp_send(req, NULL, 0);
    } else {
        ESP_LOGE(LOG_TAG, "Failed to get query parameters");

        // Send error response
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Missing required parameters: slider and value";
        httpd_resp_send(req, error_response, strlen(error_response));
    }

    return ESP_OK;
}
