#include "controller_server.h"
#include "led_config.h"
#include "led_system.h"
#include "patterns.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

#define LOG_TAG "controller_server"

static led_system_t* led_system = NULL;

// Forward declarations
// Handler functions
static esp_err_t get_groups_handler(httpd_req_t *req);
static esp_err_t put_group_handler(httpd_req_t *req);
static esp_err_t get_patterns_handler(httpd_req_t *req);
static esp_err_t set_group_pattern_handler(httpd_req_t *req);

static httpd_handle_t start_webserver();
static void init_server_handlers(httpd_handle_t *server);

// Utility functions
static esp_err_t get_url_query_param(httpd_req_t *req, const char *param_name, char *param_value, size_t param_value_size);
static esp_err_t extract_body(httpd_req_t *req, char **body_buffer);

// Create body utility functions
static void create_group_json(cJSON* parent, uint8_t group_index);

static const httpd_uri_t get_group_config_uri = {
    .uri       = "/groups/config",               // the address at which the resource can be found
    .method    = HTTP_GET,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = get_groups_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

static const httpd_uri_t put_group_uri = {
    .uri       = "/groups",               // the address at which the resource can be found
    .method    = HTTP_PUT,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = put_group_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

static const httpd_uri_t get_patterns_uri = {
    .uri       = "/patterns",               // the address at which the resource can be found
    .method    = HTTP_GET,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = get_patterns_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

static const httpd_uri_t set_group_parttern_uri = {
    .uri       = "/groups/pattern",               // the address at which the resource can be found
    .method    = HTTP_PUT,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = set_group_pattern_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

// Initialize URI handlers
static void init_server_handlers(httpd_handle_t *server) {
    httpd_register_uri_handler(*server, &get_group_config_uri);
    httpd_register_uri_handler(*server, &put_group_uri);
    httpd_register_uri_handler(*server, &get_patterns_uri);
    httpd_register_uri_handler(*server, &set_group_parttern_uri);
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

static void stop_webserver(httpd_handle_t server) {
    if (server) {
        httpd_stop(server);
        ESP_LOGI(LOG_TAG, "HTTP server stopped");
    }
}

void init_http_server(wifi_ap_status_t* wifi_ap_status, led_system_t* global_led_system)
{
    while(!wifi_ap_status->network_ready) {
        ESP_LOGW(LOG_TAG, "WiFi AP not started, HTTP server may not be reachable");
        vTaskDelay(pdMS_TO_TICKS(2000));   
    }

    led_system = global_led_system;
    if(led_system == NULL) {
        ESP_LOGE(LOG_TAG, "LED system is NULL, cannot start HTTP server");
        return;
    }

    httpd_handle_t server = start_webserver();
    if (server) {
        init_server_handlers(&server);
    } else {
        ESP_LOGE(LOG_TAG, "Failed to start HTTP server");
    }
}

// Handle GET /groups
static esp_err_t get_groups_handler(httpd_req_t *req)
{
    ESP_LOGI(LOG_TAG, "GET /groups/config handler called");

    cJSON *response_json = cJSON_CreateObject();
    cJSON *groups = cJSON_CreateArray();

    for(int i = 0; i < led_strip_config_count; i++) {
        cJSON *group_json = cJSON_CreateObject();
        create_group_json(group_json, i); 
        cJSON_AddItemToArray(groups, group_json);
    }

    cJSON_AddItemToObject(response_json, "groups", groups);
    
    char *response_str = cJSON_PrintUnformatted(response_json);
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    cJSON_Delete(response_json);
    free(response_str);
    return ESP_OK;
}

// Handle PUT /group with JSON body { id: 0, name: 'Group 1', brightnessParam: 0, speedParam: 0, params: defaultParams }
static esp_err_t put_group_handler(httpd_req_t *req)
{
    ESP_LOGI(LOG_TAG, "POST /groups handler called");

    char *buf = NULL;
    esp_err_t err = extract_body(req, &buf);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to extract body");
        return err;
    }

    if(buf == NULL) {
        ESP_LOGE(LOG_TAG, "Body buffer is NULL");
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Empty body";
        httpd_resp_send(req, error_response, strlen(error_response));
        return ESP_ERR_INVALID_ARG;
    }

    // Request data received
    ESP_LOGI(LOG_TAG, "Data received: %s", buf);

    cJSON *json = cJSON_Parse(buf);
    uint8_t group_id = cJSON_GetObjectItemCaseSensitive(json, "id")->valueint;
    
    ESP_LOGI(LOG_TAG, "Getting data for group %d", group_id);

    if (led_system && group_id < 4) {
        led_system->controller_state->group_states[group_id].brightness = cJSON_GetObjectItemCaseSensitive(json, "brightnessParam")->valuedouble / 100.0f;
        led_system->controller_state->group_states[group_id].speed = cJSON_GetObjectItemCaseSensitive(json, "speedParam")->valuedouble / 100.0f;
        cJSON *params_array = cJSON_GetObjectItemCaseSensitive(json, "params");

        int param_count = cJSON_GetArraySize(params_array);
        for (int i = 0; i < param_count && i < 10; i++) {
            cJSON *param_item = cJSON_GetArrayItem(params_array, i);
            if (cJSON_IsObject(param_item)) {
                uint8_t param_id = cJSON_GetObjectItemCaseSensitive(param_item, "id")->valueint;
                led_system->controller_state->group_states[group_id].params[param_id] = cJSON_GetObjectItemCaseSensitive(param_item, "value")->valuedouble;
            }
        }
                 
        led_system->controller_state->group_states[group_id].changed = true;
        httpd_resp_set_status(req, "200 OK");
        const char* success_response = "Group updated successfully";
        httpd_resp_send(req, success_response, strlen(success_response));
    } else {
        ESP_LOGW(LOG_TAG, "Invalid group data");
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Invalid group data";
        httpd_resp_send(req, error_response, strlen(error_response));
    }

    cJSON_Delete(json);
    free(buf);
    return ESP_OK;
}


static esp_err_t get_patterns_handler(httpd_req_t *req)
{
    ESP_LOGI(LOG_TAG, "GET /patterns handler called");

    cJSON *response_json = cJSON_CreateObject();
    cJSON *patterns = cJSON_CreateArray();

    for(int i = 0; i < get_total_pattern_count(); i++) {
        cJSON *pattern_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(pattern_json, "id", i);
        cJSON_AddStringToObject(pattern_json, "name", get_pattern_name(i));
        cJSON_AddItemToArray(patterns, pattern_json);
    }

    cJSON_AddItemToObject(response_json, "patterns", patterns);
    
    char *response_str = cJSON_PrintUnformatted(response_json);
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    cJSON_Delete(response_json);
    free(response_str);
    return ESP_OK;
}

static esp_err_t set_group_pattern_handler(httpd_req_t *req)
{
    ESP_LOGI(LOG_TAG, "PUT /groups/pattern handler called");

    char *buf = NULL;
    esp_err_t err = extract_body(req, &buf);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to extract body");
        return err;
    }

    if(buf == NULL) {
        ESP_LOGE(LOG_TAG, "Body buffer is NULL");
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Empty body";
        httpd_resp_send(req, error_response, strlen(error_response));
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(LOG_TAG, "Data received: %s", buf);

    cJSON *json = cJSON_Parse(buf);
    uint8_t group_id = cJSON_GetObjectItemCaseSensitive(json, "groupId")->valueint;
    uint8_t pattern_id = cJSON_GetObjectItemCaseSensitive(json, "patternId")->valueint;

    ESP_LOGI(LOG_TAG, "Setting pattern %d for group %d", pattern_id, group_id);

    if (led_system && group_id < 4) {
        update_pattern_state_pattern_id(led_system, group_id, pattern_id);

        cJSON *group_json = cJSON_CreateObject();
        create_group_json(group_json, group_id);

        ESP_LOGI(LOG_TAG, "Pattern %d set for group %d", pattern_id, group_id);
        httpd_resp_set_status(req, "200 OK");
        const char* json_response = cJSON_PrintUnformatted(group_json);
        httpd_resp_send(req, json_response, strlen(json_response));

        cJSON_Delete(group_json);
        free(json_response);
    } else {
        ESP_LOGW(LOG_TAG, "Invalid group or pattern data");
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Invalid group or pattern data";
        httpd_resp_send(req, error_response, strlen(error_response));
    }

    cJSON_Delete(json);
    free(buf);
    return ESP_OK;
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

// Helper function: extract body from HTTP request
// params: httpd_req_t *req - the HTTP request
//         char *body_buffer - uninitialized pointer to store the allocated body buffer
// returns: ESP_OK on success, error code on failure
static esp_err_t extract_body(httpd_req_t *req, char **body_buffer){

    // Get content length
    int total_len = req->content_len;
    if (total_len <= 0 || total_len > 1024) {
        ESP_LOGW(LOG_TAG, "Invalid content length");
        httpd_resp_set_status(req, "400 Bad Request");
        const char* error_response = "Invalid content length";
        httpd_resp_send(req, error_response, strlen(error_response));
        return ESP_ERR_INVALID_SIZE;
    }

    // Allocate buffer for body
    *body_buffer = malloc(total_len + 1);
    if (*body_buffer == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to allocate memory for request body");
        httpd_resp_set_status(req, "500 Internal Server Error");
        const char* error_response = "Memory allocation failed";
        httpd_resp_send(req, error_response, strlen(error_response));
        return ESP_ERR_NO_MEM;
    }

    // Read the body
    int ret = httpd_req_recv(req, *body_buffer, total_len);
    if (ret <= 0) {
        ESP_LOGE(LOG_TAG, "Failed to read request body");
        free(*body_buffer);
        httpd_resp_set_status(req, "500 Internal Server Error");
        const char* error_response = "Failed to read request body";
        httpd_resp_send(req, error_response, strlen(error_response));
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(LOG_TAG, "Request body extracted successfully");
    (*body_buffer)[ret] = '\0'; // Null-terminate the buffer
    return ESP_OK;
}

// Create group json for a given group index
static void create_group_json(cJSON* parent, uint8_t group_index) {
    const led_strip_config_t* config = &led_strip_configs[group_index];
    const group_state_t* state = &led_system->controller_state->group_states[group_index];

    cJSON_AddNumberToObject(parent, "id", group_index);
    cJSON_AddStringToObject(parent, "name", config->name);
    cJSON_AddNumberToObject(parent, "patternId", led_system->pattern_states[group_index].pattern_id);
    cJSON_AddNumberToObject(parent, "brightnessParam", state->brightness * 100.0f); // Scale to 0-100 for UI
    cJSON_AddNumberToObject(parent, "speedParam", state->speed * 100.0f); // Scale to 0-100 for UI
    cJSON *params = cJSON_CreateArray();
    uint8_t param_count = get_pattern_param_count(led_system->pattern_states[group_index].pattern_id);
    pattern_param_t *pattern_params = get_pattern_params(led_system->pattern_states[group_index].pattern_id);

    for(int i = 0; i < param_count; i++) {

        cJSON *param_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(param_json, "id", i);
        cJSON_AddStringToObject(param_json, "name", pattern_params[i].name);
        cJSON_AddNumberToObject(param_json, "value", state->params[i]);
        cJSON_AddNumberToObject(param_json, "type", pattern_params[i].type);
        cJSON_AddNumberToObject(param_json, "maxValue", pattern_params[i].max_value);
        cJSON_AddItemToArray(params, param_json);
    }

    cJSON_AddItemToObject(parent, "params", params);    
}