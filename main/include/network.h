
#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>

typedef struct {
    bool eth_link_up;
    bool got_ip;
    bool network_ready;
} network_status_t;

typedef struct {
    bool wifi_ap_up;
    bool network_ready;
} wifi_ap_status_t;

void init_ethernet(network_status_t* network_status);
void init_ethernet_static_ip(char* static_ip, char* gateway, char* netmask, bool enable_dns, network_status_t* network_status);
void init_wifi(wifi_ap_status_t* wifi_ap_status);

#endif // NETWORK_H