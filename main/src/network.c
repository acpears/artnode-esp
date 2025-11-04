#include "network.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "ethernet_init.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lwip/sockets.h"
#include <string.h>

#include "injected/esp_wifi.h"
#include "esp_wifi_remote.h"
#include "esp_hosted.h"

#define LOG_TAG "network"

static char* ip = "192.168.0.2";
static char* gwy = "192.168.0.1";
static char* nmsk = "255.255.255.0";

static void update_network_ready_status(network_status_t* status) {
    bool prev_ready = status->network_ready;
    status->network_ready = status->eth_link_up && status->got_ip;
    
    if (!prev_ready && status->network_ready) {
        ESP_LOGI(LOG_TAG, "🟢 Network is up");
    } else if (prev_ready && !status->network_ready) {
        ESP_LOGI(LOG_TAG, "🔴 Network is down");
    }
}


static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(LOG_TAG, "Ethernet Got IP Address: " IPSTR, IP2STR(&ip_info->ip));

    // Update flag
    network_status_t *network_status = (network_status_t *)arg;
    network_status->got_ip = true;
    update_network_ready_status(network_status);
}

static void lost_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ESP_LOGI(LOG_TAG, "Ethernet Lost IP Address");

    // Update flag
    network_status_t *network_status = (network_status_t *)arg;
    network_status->got_ip = false;
    update_network_ready_status(network_status);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    network_status_t *network_status = (network_status_t *)arg;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(LOG_TAG, "Ethernet Link Up");
        network_status->eth_link_up = true;
        update_network_ready_status(network_status);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "Ethernet Link Down");
        network_status->eth_link_up = false;
        update_network_ready_status(network_status);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(LOG_TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(LOG_TAG, "Ethernet Stopped");
        network_status->eth_link_up = false;
        update_network_ready_status(network_status);
        break;
    default:
        break;
    }
}



// Set static IP configuration for ethernet
static void set_eth_static_ip(esp_netif_t *netif, bool enable_dns)
{
    esp_netif_ip_info_t ip_info;
    
    // Stop DHCP client
    esp_netif_dhcpc_stop(netif);
    
    // Set static IP configuration
    esp_netif_str_to_ip4(ip, &ip_info.ip);      // Static IP
    esp_netif_str_to_ip4(gwy, &ip_info.gw);       // Gateway
    esp_netif_str_to_ip4(nmsk, &ip_info.netmask); // Netmask

    esp_err_t ret = esp_netif_set_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set static IP: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOG_TAG, "Static IP set to 192.168.0.20");
    }
    
    if(!enable_dns) return;
    ESP_LOGI(LOG_TAG, "Setting DNS servers");
    esp_netif_dns_info_t dns;
    IP4_ADDR(&dns.ip.u_addr.ip4, 8, 8, 8, 8);  // Google DNS
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);
    IP4_ADDR(&dns.ip.u_addr.ip4, 8, 8, 4, 4);  // Google DNS backup
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns);
}

// Initialize Ethernet with TCP/IP stack
static void init_eth_tcp(bool use_static_ip, network_status_t* network_status) {
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_port_cnt));

    // Create new netif instance for Ethernet and attach the Ethernet driver
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(*eth_handles)));
    if (use_static_ip) {
        set_eth_static_ip(eth_netif, false);
    }

    // Register application event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler, network_status));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, lost_ip_event_handler, network_status));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, network_status));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(*eth_handles));
}

void init_ethernet(network_status_t* network_status) {
    init_eth_tcp(false, network_status);
}

void init_ethernet_static_ip(char* static_ip, char* gateway, char* netmask, bool enable_dns, network_status_t* network_status) {
    ip = static_ip;
    gwy = gateway;
    nmsk = netmask;

    init_eth_tcp(true, network_status);
}

// WiFi
#define ESP_WIFI_SSID "esp32_ap"
#define ESP_WIFI_PASS "password123"
#define ESP_WIFI_CHANNEL 10
#define MAX_STA_CONN 1
#define AP_IP_ADDR "192.168.10.1"

static wifi_config_t wifi_config = {
    .ap = {
        .ssid = ESP_WIFI_SSID,
        .ssid_len = strlen(ESP_WIFI_SSID),
        .ssid_hidden = 0,
        .channel = ESP_WIFI_CHANNEL,
        .password = ESP_WIFI_PASS,
        .max_connection = MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
            .required = true,
        },
    },
};

static void update_wifi_ap_ready_status(wifi_ap_status_t* status) {
    bool prev_ready = status->network_ready;
    status->network_ready = status->wifi_ap_up;

    if (!prev_ready && status->network_ready) {
        ESP_LOGI(LOG_TAG, "🟢 WiFi AP %s is up", ESP_WIFI_SSID);
    } else if (prev_ready && !status->network_ready) {
        ESP_LOGI(LOG_TAG, "🔴 WiFi AP %s is down", ESP_WIFI_SSID);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    wifi_ap_status_t *wifi_ap_status = (wifi_ap_status_t *)arg;

    switch (event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(LOG_TAG, "WiFi AP has started");
            wifi_ap_status->wifi_ap_up = true;
            update_wifi_ap_ready_status(wifi_ap_status);
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(LOG_TAG, "WiFi AP has stopped");
            wifi_ap_status->wifi_ap_up = false;
            update_wifi_ap_ready_status(wifi_ap_status);
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(LOG_TAG, "Station " MACSTR " has connected to AP and is assigned AID=%d",
                     MAC2STR(((wifi_event_ap_staconnected_t*)event_data)->mac),
                     ((wifi_event_ap_staconnected_t*)event_data)->aid);
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(LOG_TAG, "Station " MACSTR " has disconnected from AP",
                     MAC2STR(((wifi_event_ap_stadisconnected_t*)event_data)->mac));
            break;
        default:
            ESP_LOGI(LOG_TAG, "WiFi Event: %ld", event_id);
            break;
    }
}

// Set static IP configuration for WiFi AP
static void set_wifi_dhcp_settings(esp_netif_t *netif)
{
    esp_err_t ret;
    
    // Stop DHCP server to set custom IP
    ret = esp_netif_dhcps_stop(netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        ESP_LOGE(LOG_TAG, "Failed to stop DHCP server: %s", esp_err_to_name(ret));
        return;
    }
    
    // Set IP for AP
    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
    
    esp_netif_str_to_ip4(AP_IP_ADDR, &ip_info.ip);
    esp_netif_str_to_ip4("255.255.255.0", &ip_info.netmask);
    esp_netif_str_to_ip4(AP_IP_ADDR, &ip_info.gw);

    // Set the IP info
    ret = esp_netif_set_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set IP info: %s", esp_err_to_name(ret));
        return;
    }

    // Set DHCP option for subnet mask
    ret = esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_SUBNET_MASK, &ip_info.netmask, sizeof(ip_info.netmask));
    if (ret != ESP_OK) {
        ESP_LOGW(LOG_TAG, "Failed to set DHCP subnet mask option: %s", esp_err_to_name(ret));
        // Continue anyway, this is not critical
    }

    // Restart DHCP server
    ret = esp_netif_dhcps_start(netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
        ESP_LOGE(LOG_TAG, "Failed to start DHCP server: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(LOG_TAG, "WiFi AP IP configured: %s", AP_IP_ADDR);
}


static void init_wifi_tcp(wifi_ap_status_t* wifi_ap_status) {
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t *wifi_ap_netif = esp_netif_create_default_wifi_ap();
    set_wifi_dhcp_settings(wifi_ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // always start with this
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register application event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, wifi_ap_status));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(LOG_TAG, "WiFi Access Point \"%s\" started", ESP_WIFI_SSID);
}

void init_wifi(wifi_ap_status_t* wifi_ap_status) {
    init_wifi_tcp(wifi_ap_status);
}