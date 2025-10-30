#include "network.h"
#include "handlers.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ethernet_init.h"
#include "lwip/sockets.h"

#define CONFIG_LOG_MAXIMUM_LEVEL 3
#define LOG_TAG "network"

static char* ip = "192.168.0.2";
static char* gwy = "192.168.0.1";
static char* nmsk = "255.255.255.0";

static void set_static_ip(esp_netif_t *netif, bool enable_dns)
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

static void init_eth_tcp(bool use_static_ip, bool* got_ip_flag, bool* eth_link_up_flag) {
    // Initialize TCP/IP stack and the event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_port_cnt));

    // Create new netif instance for Ethernet and attach the Ethernet driver
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(*eth_handles)));
    if (use_static_ip) {
        set_static_ip(eth_netif, false);
    }

    // Register application event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler, got_ip_flag));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, IP_EVENT_ETH_LOST_IP, lost_ip_event_handler, got_ip_flag));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, eth_link_up_flag));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(*eth_handles));
}

void init_network(bool* got_ip_flag, bool* eth_link_up_flag) {
    init_eth_tcp(false, got_ip_flag, eth_link_up_flag);
}

void init_network_static_ip(char* static_ip, char* gateway, char* netmask, bool enable_dns, bool* eth_link_up_flag, bool* got_ip_flag) {
    ip = static_ip;
    gwy = gateway;
    nmsk = netmask;

    init_eth_tcp(true, got_ip_flag, eth_link_up_flag);
}
