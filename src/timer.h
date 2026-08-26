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
void timer_reset_fired(int index);
bool timer_get(int index, timer_config_t *out);
bool timer_set(int index, const timer_config_t *cfg);
int8_t timer_check(unsigned long current_epoch, int timezone_offset);
void timer_get_next(unsigned long current_epoch, int timezone_offset,
                    unsigned long *next_epoch, uint8_t *next_action);
void timer_to_json(JsonDocument &doc);
bool timer_from_json(JsonDocument &doc);

// --- Cyclic timer ---

typedef struct __attribute__((packed)) {
    uint8_t  enabled;
    uint16_t duration_min;
} cyclic_config_t;

int8_t cyclic_check(unsigned long now_ms, bool current_relay_state);
void   cyclic_reset(void);
bool   cyclic_get_enabled(void);
uint16_t cyclic_get_duration(void);
void   cyclic_set_enabled(bool enabled);
void   cyclic_set_duration(uint16_t min);

// --- LittleFS persistence ---

bool timer_save_littlefs(void);
bool timer_load_littlefs(void);

// --- Pulse config (persistence + runtime) ---

bool     timer_pulse_get_enabled(void);
uint16_t timer_pulse_get_duration(void);
void     timer_pulse_set_enabled(bool enabled);
void     timer_pulse_set_duration(uint16_t min);
void     pulse_start(void);
void     pulse_cancel(void);
int8_t   pulse_check(unsigned long now_ms);

#endif
