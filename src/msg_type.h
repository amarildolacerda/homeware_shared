#ifndef HW_SHARED_MSG_TYPE_H
#define HW_SHARED_MSG_TYPE_H

#include <stdint.h>

enum msg_type_t : uint8_t {
    MSG_SENSOR_DATA     = 0x01,
    MSG_PAIR_REQUEST    = 0x02,
    MSG_PAIR_RESPONSE   = 0x03,
    MSG_ACK             = 0x04,
    MSG_HEARTBEAT       = 0x05,
    MSG_OTA_TRIGGER     = 0x06,
    MSG_COMMAND         = 0x07,
    MSG_TIME_SYNC       = 0x08,
    MSG_GW_ANNOUNCE     = 0x09,
    MSG_GW_DISCOVER     = 0x0A,
    MSG_REPEATER_STATUS = 0x0B,
    MSG_RESTART         = 0x0C,
    MSG_NAK             = 0x0D,
};

#endif
