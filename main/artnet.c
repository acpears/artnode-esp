#include <stdint.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#define ARTNET_ID "Art-Net"
#define ARTNET_OP_DMX 0x5000
#define ARTNET_PROTOCOL_VERSION 0x0E00 // Protocol version 14

#define ADDRESS_FAMILY AF_INET
#define IP_PROTOCOL IPPROTO_IP

#define LOG_TAG "artnet-module"

typedef struct artnet_packet {
    uint8_t id[8];              // ID "Art-Net"
    uint16_t op_code;           // OpCode
    uint16_t prot_ver;          // Protocol Version
    uint8_t sequence;           // Packet sequence
    uint8_t physical;           // Physical port
    uint16_t universe;          // Universe
    uint16_t length;            // Length of data
    uint8_t data[512];          // DMX data
} __attribute__((packed)) artnet_packet_t;

static struct sockaddr_in dest_addr;
static int artnet_socket = -1;

static void set_dest_addr(const char *ip, uint16_t port) {
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
}

void artnet_init(char * destination_ip) {
    // Initialize destination address
    set_dest_addr(destination_ip, 6454);
    artnet_socket = socket(ADDRESS_FAMILY, SOCK_DGRAM, IP_PROTOCOL);
    if (artnet_socket < 0) {
        // Handle socket creation error
    }

    // Set send and receive timeouts
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt (artnet_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    setsockopt (artnet_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

}

static void set_arnet_packet_static_content(artnet_packet_t *packet) {
    // Set ID to "Art-Net"
    const char artnet_id[8] = ARTNET_ID;
    for (int i = 0; i < 8; i++) {
        packet->id[i] = artnet_id[i];
    }

    // Set OpCode for ArtDMX (0x5000)
    packet->op_code = ARTNET_OP_DMX;

    // Set Protocol Version to 14
    packet->prot_ver = ARTNET_PROTOCOL_VERSION;

    // Set sequence and physical to 0
    packet->sequence = 0;
    packet->physical = 0;
}

static void create_artnet_packet(artnet_packet_t *packet, uint16_t universe, const uint8_t *dmx_data, uint16_t dmx_length) {
    set_arnet_packet_static_content(packet);

    // Set universe
    packet->universe = universe;

    // Set length and copy DMX data
    if (dmx_length > 512) {
        dmx_length = 512; // Limit to max DMX length
    }
    packet->length = dmx_length;
    for (int i = 0; i < dmx_length; i++) {
        packet->data[i] = dmx_data[i];
    }
}

void artnet_send_dmx(uint16_t universe, const uint8_t *dmx_data, uint16_t dmx_length) {
    if (artnet_socket < 0) {
        ESP_LOF(LOG_TAG, "Art-Net socket not initialized");
        return;
    }

    artnet_packet_t packet;
    create_artnet_packet(&packet, universe, dmx_data, dmx_length);

    // Send the packet
    ssize_t sent_bytes = sendto(artnet_socket, &packet, sizeof(artnet_packet_t), 0,
                                (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent_bytes < 0) {
        // Handle send error
    }
}

