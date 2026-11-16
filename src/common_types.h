#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>

#define BATTERY_LEVEL_UNAVAILABLE 0xFF  // No battery telemetry is available.

typedef struct __attribute__((packed)) {
    uint8_t state;
} payload_onoff_t;

#endif
