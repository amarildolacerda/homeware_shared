#ifndef HW_SHARED_COMMON_REPEATER_H
#define HW_SHARED_COMMON_REPEATER_H

#include <stdint.h>

#define REPEATER_MAX_CLIENTS 5

typedef struct {
    uint8_t mac[6];
    uint32_t pkt_count;
} repeater_client_t;

bool repeater_init(uint16_t eeprom_addr);
void repeater_forward(const uint8_t *src_mac, const uint8_t *data, int len,
                      const uint8_t *gateway_mac, const uint8_t *broadcast_mac,
                      void (*send_fn)(const uint8_t *mac, const uint8_t *data, int len, const char *tag),
                      const char *tag);
bool repeater_is_enabled(void);
void repeater_set_enabled(bool enabled);
void repeater_save_enable(void);
void repeater_load_enable(void);
uint32_t repeater_get_fwd_count(void);
const repeater_client_t* repeater_get_clients(void);
int repeater_get_client_count(void);
void repeater_reset_stats(void);

#endif
