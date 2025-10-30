#ifndef ARTNET_H
#define ARTNET_H
#include <stdint.h>

#define DMX_DATA_MAX_LENGTH 512

int artnet_init(char * destination_ip);
int artnet_send_dmx(uint16_t universe, const uint8_t *dmx_data, uint16_t dmx_data_length);
int artnet_send_dmx_array(const uint8_t dmx_data_array[][DMX_DATA_MAX_LENGTH], uint16_t num_universes);
int artnet_close();
#endif // ARTNET_H

