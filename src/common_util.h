#ifndef HW_SHARED_UTIL_H
#define HW_SHARED_UTIL_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"

// Device ID — centralizado ("agri_XXXXXX" a partir do chip_id).
// Retorna ponteiro para buffer estático; válido até próxima chamada.
const char* getDeviceId();

// MAC utilities — used by ESP-NOW, TCP, and any code that deals with MACs.
static inline void mac_to_str(const uint8_t *mac, char *buf, size_t len) {
    snprintf(buf, len, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static inline bool mac_equal(const uint8_t *a, const uint8_t *b) {
    return memcmp(a, b, 6) == 0;
}

static inline void mac_copy(uint8_t *dst, const uint8_t *src) {
    memcpy(dst, src, 6);
}

// Converte uptime (ms) para string "Xd Yh Zm Ws" (usado em dashboards/console).
void uptime_to_str(unsigned long ms, char *buf, size_t len);

// Versao String (Arduino).
void uptime_to_str(unsigned long ms, String &out);

#endif
