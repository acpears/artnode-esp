
#include <stdbool.h>

void init_network(bool* got_ip_flag, bool* eth_link_up_flag);

void init_network_static_ip(char* static_ip, char* gateway, char* netmask, bool enable_dns, bool* eth_link_up_flag, bool* got_ip_flag);