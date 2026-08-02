#ifndef HW_SHARED_LORA_PROTOCOL_H
#define HW_SHARED_LORA_PROTOCOL_H

#include <stdint.h>
#include "msg_type.h"

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

typedef struct {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_id[6];
    int8_t   rssi;
    uint8_t  payload_len;
    uint8_t  command;
} lora_command_t;

#pragma pack(pop)

/* Unified message types in shared/src/msg_type.h */

#define LORA_HEADER_SIZE   11
#define LORA_MAX_PAYLOAD   200

#endif
