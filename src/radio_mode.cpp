// shared/src/radio_mode.cpp
#include "radio_mode.h"
#include "shared_config.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>

static uint8_t s_active_mode = 0xFF; // 0xFF = ainda não resolvido

static bool equals_ci(const char* a, const char* b)
{
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a - 'A' + 'a') : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b - 'A' + 'a') : *b;
        if (ca != cb) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

bool radio_mode_compiled(uint8_t mode)
{
    switch (mode) {
    case RADIO_MODE_ESPNOW:
#ifdef ESPNOW_ENABLED
        return true;
#else
        return false;
#endif
    case RADIO_MODE_TCP:
#ifdef TCP_ENABLED
        return true;
#else
        return false;
#endif
    default:
        return false;
    }
}

const char* radio_mode_id(uint8_t mode)
{
    return mode == RADIO_MODE_TCP ? "tcp" : "espnow";
}

const char* radio_mode_label(uint8_t mode)
{
    return mode == RADIO_MODE_TCP ? "TCP (HTTP)" : "ESP-NOW";
}

bool radio_mode_parse(const char* text, uint8_t* out)
{
    if (!text || !out) return false;
    if (equals_ci(text, "tcp") || equals_ci(text, "http") || strcmp(text, "1") == 0) {
        *out = RADIO_MODE_TCP;
        return true;
    }
    if (equals_ci(text, "espnow") || equals_ci(text, "esp-now") || strcmp(text, "0") == 0) {
        *out = RADIO_MODE_ESPNOW;
        return true;
    }
    return false;
}

uint8_t radio_mode_load()
{
#ifdef RADIO_SELECT_ENABLED
    EEPROM.begin(EEPROM_SIZE);
    uint8_t val = EEPROM.read(EEPROM_RADIO_MODE_OFFSET);
    EEPROM.end();
    if (radio_mode_compiled(val)) return val;
#endif
    return RADIO_MODE_DEFAULT;
}

void radio_mode_save(uint8_t mode)
{
    if (!radio_mode_compiled(mode)) return;
    s_active_mode = mode;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_RADIO_MODE_OFFSET, mode);
    EEPROM.commit();
    EEPROM.end();
}

uint8_t radio_mode_active()
{
    if (s_active_mode == 0xFF) s_active_mode = radio_mode_load();
    return s_active_mode;
}

void radio_mode_set_active(uint8_t mode)
{
    s_active_mode = mode;
}
