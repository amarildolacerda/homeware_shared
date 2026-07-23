#include "common_repeater.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>

#define REPEATER_DEDUP_CACHE_SIZE 10
#define REPEATER_DEDUP_TTL_MS 5000

static bool s_repeater_enabled = false;
static uint32_t s_repeater_fwd = 0;
static repeater_client_t s_rep_clients[REPEATER_MAX_CLIENTS];
static int s_rep_client_num = 0;
static uint16_t s_eeprom_addr = 0;

typedef struct {
    uint8_t src_mac[6];
    uint16_t sequence;
    unsigned long timestamp;
} dedup_entry_t;

static dedup_entry_t s_dedup_cache[REPEATER_DEDUP_CACHE_SIZE];
static int s_dedup_idx = 0;

static bool dedup_check(const uint8_t *src_mac, uint16_t sequence) {
    unsigned long now = millis();
    for (int i = 0; i < REPEATER_DEDUP_CACHE_SIZE; i++) {
        if (s_dedup_cache[i].timestamp == 0) continue;
        if ((now - s_dedup_cache[i].timestamp) > REPEATER_DEDUP_TTL_MS) {
            s_dedup_cache[i].timestamp = 0;
            continue;
        }
        if (memcmp(s_dedup_cache[i].src_mac, src_mac, 6) == 0 &&
            s_dedup_cache[i].sequence == sequence) {
            return true;
        }
    }
    return false;
}

static void dedup_add(const uint8_t *src_mac, uint16_t sequence) {
    s_dedup_cache[s_dedup_idx].timestamp = millis();
    memcpy(s_dedup_cache[s_dedup_idx].src_mac, src_mac, 6);
    s_dedup_cache[s_dedup_idx].sequence = sequence;
    s_dedup_idx = (s_dedup_idx + 1) % REPEATER_DEDUP_CACHE_SIZE;
}

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
    memset(s_dedup_cache, 0, sizeof(s_dedup_cache));
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

    if (len < 4) return;
    uint16_t sequence = data[2] | (data[3] << 8);

    if (dedup_check(src_mac, sequence)) {
        Serial.printf("[%s] DEDUP drop seq=%d from %02X:%02X:%02X:%02X:%02X:%02X\n",
                      tag ? tag : "repeater", sequence,
                      src_mac[0], src_mac[1], src_mac[2],
                      src_mac[3], src_mac[4], src_mac[5]);
        return;
    }

    dedup_add(src_mac, sequence);

    bool from_gateway = (memcmp(src_mac, gateway_mac, 6) == 0);
    if (!from_gateway) {
        track_client(src_mac);
    }
    send_fn(broadcast_mac, data, len, tag);
    s_repeater_fwd++;
}
