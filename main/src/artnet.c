#include "artnet.h"

#include <stdint.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#define ARTNET_ID "Art-Net"
#define ARTNET_OP_DMX 0x5000
#define ARTNET_PROTOCOL_VERSION 0x0E00 // Protocol version 14
#define ARTNET_PORT 6454

#define ADDRESS_FAMILY AF_INET
#define IP_PROTOCOL IPPROTO_IP

#define LOG_TAG "artnet"

// Add this static variable near the top of the file
static int64_t last_print_time = 0;

typedef struct {
    int sock_fd;
    struct sockaddr_in dest_addr;
} artnet_socket_t; 

static artnet_socket_t artnet_socket = {0};

typedef struct artnet_packet {
    uint8_t id[8];              // ID "Art-Net"
    uint16_t op_code;           // OpCode
    uint16_t prot_ver;          // Protocol Version
    uint8_t sequence;           // Packet sequence
    uint8_t physical;           // Physical port
    uint16_t universe;          // Universe
    uint16_t length;            // Length of data
    uint8_t data[DMX_DATA_MAX_LENGTH];          // DMX data
} __attribute__((packed)) artnet_packet_t;

static void set_socket_dest_addr(const char *ip, uint16_t port) {
    artnet_socket.dest_addr.sin_addr.s_addr = inet_addr(ip);
    artnet_socket.dest_addr.sin_family = AF_INET;
    artnet_socket.dest_addr.sin_port = htons(port);
}

// Print data array in hex format on the same line separating bytes with spaces
static void print_data_array(const uint8_t *data, uint16_t length) {
    ESP_LOGI(LOG_TAG, "Data (%d bytes):", length);
    char buffer[9 * DMX_DATA_MAX_LENGTH + 1]; // Each byte takes 2 chars + space, plus null terminator
    char *ptr = buffer;
    for (int i = 0; i < length; i++) {
        uint64_t written = sprintf(ptr, "%i:%02X | ", i+1, data[i]);
        ptr += written;
    }
    *(ptr - 1) = '\0'; // Replace last space with null terminator
    ESP_LOGI(LOG_TAG, "%s", buffer);
}

// Pretty print packet
static void print_artnet_packet(const artnet_packet_t *packet) {
    ESP_LOGI(LOG_TAG, "Art-Net Packet:");
    ESP_LOGI(LOG_TAG, "ID: %.8s", packet->id);
    ESP_LOGI(LOG_TAG, "OpCode: 0x%04X", packet->op_code);
    ESP_LOGI(LOG_TAG, "Protocol Version: %d", ntohs(packet->prot_ver));
    ESP_LOGI(LOG_TAG, "Sequence: %d", packet->sequence);
    ESP_LOGI(LOG_TAG, "Physical: %d", packet->physical);
    ESP_LOGI(LOG_TAG, "Universe: %d", packet->universe);
    ESP_LOGI(LOG_TAG, "Length: %d", ntohs(packet->length));
    print_data_array(packet->data, ntohs(packet->length));
}

int artnet_init(char * destination_ip) {
    // Initialize destination address and socket
    set_socket_dest_addr(destination_ip, ARTNET_PORT);
    artnet_socket.sock_fd = socket(ADDRESS_FAMILY, SOCK_DGRAM, IP_PROTOCOL);
    if (artnet_socket.sock_fd < 0) {
        ESP_LOGE(LOG_TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(LOG_TAG, "Socket created for %s:%d", destination_ip, ARTNET_PORT);


    // SOCKET OPTIONS
    int send_buffer_size = 550;  // 550 for multiple Art-Net packets
    int recv_buffer_size = 100;   // 100 bytes for incoming data
    setsockopt(artnet_socket.sock_fd, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size));
    setsockopt(artnet_socket.sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, sizeof(recv_buffer_size));

    // Set send and receive timeouts
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt (artnet_socket.sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    setsockopt (artnet_socket.sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    return 0;
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

static void build_artnet_packet(artnet_packet_t *packet, uint8_t universe, const uint8_t *dmx_data, uint16_t dmx_data_length) {
    set_arnet_packet_static_content(packet);
    
    // Set universe
    packet->universe = universe; // Convert to network byte order

    // Set length and copy DMX data
    if (dmx_data_length > DMX_DATA_MAX_LENGTH) {
        dmx_data_length = DMX_DATA_MAX_LENGTH; // Limit to max DMX length
    }

    // Set length and copy DMX data
    packet->length = htons(dmx_data_length); // Convert to network byte order
    for (int i = 0; i < dmx_data_length; i++) {
        packet->data[i] = dmx_data[i];
    }
}

int artnet_send_dmx(uint8_t universe, const uint8_t *dmx_data, uint16_t dmx_data_length) {
    if (artnet_socket.sock_fd < 0) {
        ESP_LOGE(LOG_TAG, "Art-Net socket not initialized!!!");
        return -1;
    }


    artnet_packet_t packet = {0};
    build_artnet_packet(&packet, universe, dmx_data, dmx_data_length);

    size_t packet_size = sizeof(artnet_packet_t) - DMX_DATA_MAX_LENGTH + dmx_data_length;

    if(CONFIG_LOG_MAXIMUM_LEVEL > 3) {
        int64_t current_time = esp_timer_get_time(); // Get time in microseconds
        if (current_time - last_print_time >= 1000000) { // 1 second = 1,000,000 microseconds
            print_artnet_packet(&packet);
            last_print_time = current_time;
        }
    }
    ssize_t sent_bytes = sendto(artnet_socket.sock_fd, &packet, packet_size, 0,
                                (struct sockaddr *)&artnet_socket.dest_addr, sizeof(artnet_socket.dest_addr));
    if (sent_bytes < 0) {
        ESP_LOGE(LOG_TAG, "Error sending artnet packet: errno %d", errno);
        return -1;
    }
    return 0;
}

// arnet send which takes a n x 512 byte dmx data array
int artnet_send_dmx_array(const uint8_t dmx_data_array[][DMX_DATA_MAX_LENGTH], uint8_t num_universes_to_send) {
    for (uint8_t i = 0; i < num_universes_to_send; i++) {
        if (artnet_send_dmx(i, dmx_data_array[i], DMX_DATA_MAX_LENGTH) < 0) {
            ESP_LOGE(LOG_TAG, "Failed to send DMX data for universe %d", i);
            return -1;
        }
    }
    return 0;
}

int artnet_close() {
    if (artnet_socket.sock_fd >= 0) {
        close(artnet_socket.sock_fd);
        artnet_socket.sock_fd = -1;
    }
    return 0;
}
