#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "esp_netif_net_stack.h"
#include "esp_eth.h"
#include "ethernet_init.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define HOST_IP_ADDR "192.168.0.10"
#define PORT 8080

static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";

static bool got_ip = false;

static void set_static_ip(esp_netif_t *netif, bool enable_dns)
{
    esp_netif_ip_info_t ip_info;
    
    // Stop DHCP client
    esp_netif_dhcpc_stop(netif);
    
    // Set static IP configuration
    IP4_ADDR(&ip_info.ip, 192, 168, 0, 2);      // Static IP
    IP4_ADDR(&ip_info.gw, 192, 168, 0, 1);       // Gateway
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0); // Netmask
    
    esp_err_t ret = esp_netif_set_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set static IP: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Static IP set to 192.168.0.100");
    }
    
    if(!enable_dns) return;
    ESP_LOGI(TAG, "Setting DNS servers");
    esp_netif_dns_info_t dns;
    IP4_ADDR(&dns.ip.u_addr.ip4, 8, 8, 8, 8);  // Google DNS
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);
    IP4_ADDR(&dns.ip.u_addr.ip4, 8, 8, 4, 4);  // Google DNS backup
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    uint8_t eth_port = (uint32_t) arg;
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Port %d Link Up", eth_port);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Port %d Link Down", eth_port);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Port %d Started", eth_port);
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Port %d Stopped", eth_port);
        break;
    default:
        break;
    }
}

/* Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    got_ip = true;
}

// Initialize Ethernet
static void init_eth(bool enable_static_ip){
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
    if (enable_static_ip) {
        set_static_ip(eth_netif, false);
    }

    // Register application event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(*eth_handles));
}

static void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    while (1) {
        if (!got_ip) {
            ESP_LOGI(TAG, "Waiting for IP address...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);

        while (1) {

            int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "Message sent");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
                if (strncmp(rx_buffer, "OK: ", 4) == 0) {
                    ESP_LOGI(TAG, "Received expected message, reconnecting");
                    break;
                }
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    init_eth(false);

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
}
