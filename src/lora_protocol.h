#ifndef HW_SHARED_LORA_PROTOCOL_H
#define HW_SHARED_LORA_PROTOCOL_H

#include <stdint.h>
#include "espnow_protocol.h"

#pragma pack(push, 1)

typedef struct {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_id[6];
    int8_t   rssi;
    uint8_t  payload_len;
    uint8_t  payload[];
} lora_frame_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_id[6];
    int8_t   rssi;
    uint8_t  payload_len;
    uint8_t  assigned_slot;
} lora_pair_response_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_id[6];
    int8_t   rssi;
    uint8_t  payload_len;
    uint8_t  reason;
} lora_nak_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_id[6];
    int8_t   rssi;
    uint8_t  payload_len;
    uint8_t  sensor_type;
    char     device_name[16];
} lora_pair_request_t;

#pragma pack(pop)

enum lora_msg_type_t {
    LORA_MSG_SENSOR_DATA   = 0x01,
    LORA_MSG_PAIR_REQUEST  = 0x02,
    LORA_MSG_PAIR_RESPONSE = 0x03,
    LORA_MSG_HEARTBEAT     = 0x04,
    LORA_MSG_NAK           = 0x05,
    LORA_MSG_GW_ANNOUNCE   = 0x06,
    LORA_MSG_COMMAND       = 0x07,
};

#define LORA_HEADER_SIZE   11
#define LORA_MAX_PAYLOAD   200

#endif
