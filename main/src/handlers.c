#include "handlers.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_eth.h"

static const char *LOG_TAG = "handlers";

void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(LOG_TAG, "Ethernet Got IP Address");
    ESP_LOGI(LOG_TAG, "~~~~~~~~~~~");
    ESP_LOGI(LOG_TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(LOG_TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(LOG_TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(LOG_TAG, "~~~~~~~~~~~");

    // Update flag
    bool *got_ip_flag = (bool *)arg;
    *got_ip_flag = true;

}

void lost_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ESP_LOGI(LOG_TAG, "Ethernet Lost IP Address");

    // Update flag
    bool *got_ip_flag = (bool *)arg;
    *got_ip_flag = false;
}

void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    uint8_t eth_port = (uint32_t) arg;
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)data;

    // Update flag
    bool *eth_link_up = (bool *)arg;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(LOG_TAG, "Ethernet Port %d Link Up", eth_port);
        *eth_link_up = true;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "Ethernet Port %d Link Down", eth_port);
        *eth_link_up = false;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(LOG_TAG, "Ethernet Port %d Started", eth_port);
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(LOG_TAG, "Ethernet Port %d Stopped", eth_port);
        break;
    default:
        break;
    }
}