#include "common_repeater.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>

static bool s_repeater_enabled = false;
static uint32_t s_repeater_fwd = 0;
static repeater_client_t s_rep_clients[REPEATER_MAX_CLIENTS];
static int s_rep_client_num = 0;
static uint16_t s_eeprom_addr = 0;

static void track_client(const uint8_t *mac) {
    for (int i = 0; i < s_rep_client_num; i++) {
        if (memcmp(s_rep_clients[i].mac, mac, 6) == 0) {
            s_rep_clients[i].pkt_count++;
            return;
        }
    }
    if (s_rep_client_num < REPEATER_MAX_CLIENTS) {
        memcpy(s_rep_clients[s_rep_client_num].mac, mac, 6);
        s_rep_clients[s_rep_client_num].pkt_count = 1;
        s_rep_client_num++;
    }
}

bool repeater_init(uint16_t eeprom_addr) {
    s_eeprom_addr = eeprom_addr;
    repeater_load_enable();
    return true;
}

void repeater_load_enable(void) {
    EEPROM.begin(256);
    s_repeater_enabled = (EEPROM.read(s_eeprom_addr) == 1);
    EEPROM.end();
}

void repeater_save_enable(void) {
    EEPROM.begin(256);
    EEPROM.write(s_eeprom_addr, s_repeater_enabled ? 1 : 0);
    EEPROM.commit();
    EEPROM.end();
}

bool repeater_is_enabled(void) { return s_repeater_enabled; }
void repeater_set_enabled(bool enabled) { s_repeater_enabled = enabled; }
uint32_t repeater_get_fwd_count(void) { return s_repeater_fwd; }
const repeater_client_t* repeater_get_clients(void) { return s_rep_clients; }
int repeater_get_client_count(void) { return s_rep_client_num; }

void repeater_reset_stats(void) {
    s_repeater_fwd = 0;
    for (int i = 0; i < s_rep_client_num; i++)
        s_rep_clients[i].pkt_count = 0;
}

void repeater_forward(const uint8_t *src_mac, const uint8_t *data, int len,
                      const uint8_t *gateway_mac, const uint8_t *broadcast_mac,
                      void (*send_fn)(const uint8_t *mac, const uint8_t *data, int len, const char *tag),
                      const char *tag) {
    if (!s_repeater_enabled) return;

    bool from_gateway = (memcmp(src_mac, gateway_mac, 6) == 0);
    if (from_gateway) {
        send_fn(broadcast_mac, data, len, tag);
    } else {
        track_client(src_mac);
        send_fn(broadcast_mac, data, len, tag);
    }
    s_repeater_fwd++;
}
