#ifndef TCP_PROTOCOL_H
#define TCP_PROTOCOL_H

#include <stdint.h>
#include "msg_type.h"

// UDP Discovery port (same as bridge)
#define TCP_UDP_PORT 5000
#define TCP_HTTP_PORT 80

// Command queue TTL
#define TCP_COMMAND_TTL_MS 30000

// Max pending commands per device
#define TCP_MAX_PENDING_COMMANDS 10

// Max TCP clients (for reference, not used in HTTP mode)
#define TCP_MAX_CLIENTS 4

// UDP Discovery structures
struct __attribute__((packed)) tcp_gw_discover_t {
    uint8_t msg_type;     // MSG_GW_DISCOVER (0x0A)
    uint8_t sensor_type;  // sensor type
    char device_name[32]; // device name
};

struct __attribute__((packed)) tcp_gw_announce_t {
    uint8_t msg_type;      // MSG_GW_ANNOUNCE (0x09)
    uint8_t fw_version[4]; // firmware version
    char hub_ip[16];       // hub IP address
    uint16_t hub_port;     // HTTP port (80)
};

#endif
