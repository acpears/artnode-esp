
#include <stdbool.h>

typedef struct {
    bool eth_link_up;
    bool got_ip;
    bool network_ready;
} network_status_t;

void init_network(network_status_t* network_status);
void init_network_static_ip(char* static_ip, char* gateway, char* netmask, bool enable_dns, network_status_t* network_status);