#ifndef HW_SHARED_TIMER_H
#define HW_SHARED_TIMER_H

#include <stdint.h>
#include <ArduinoJson.h>

typedef struct __attribute__((packed)) {
    uint8_t  enabled;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  action;
    uint8_t  days_mask;  // bitmask 1<<wday (0=Dom ... 6=Sab); 0 = todos os dias
} timer_config_t;

bool timer_init(uint16_t eeprom_base, uint8_t max_timers);
void timer_load();
void timer_save();
bool timer_get(int index, timer_config_t *out);
bool timer_set(int index, const timer_config_t *cfg);
int8_t timer_check(unsigned long current_epoch, int timezone_offset);
void timer_get_next(unsigned long current_epoch, int timezone_offset,
                    unsigned long *next_epoch, uint8_t *next_action);
void timer_to_json(JsonDocument &doc);
bool timer_from_json(JsonDocument &doc);

#endif
