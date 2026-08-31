#include "timer.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define TIMER_LITTLEFS_FILE "/timers.json"

static timer_config_t *s_timers = nullptr;
static uint16_t *s_last_fired_minute = nullptr;
static unsigned long *s_last_fired_epoch = nullptr;
static uint16_t s_eeprom_base = 0;
static uint8_t s_max_timers = 0;
static bool s_loaded = false;

static cyclic_config_t s_cyclic = {0, 30};
static struct { bool enabled; uint16_t duration_min; } s_pulse_cfg = {false, 60};
static bool s_cyclic_waiting_off = false;
static unsigned long s_cyclic_next_event = 0;

bool timer_init(uint16_t eeprom_base, uint8_t max_timers) {
    s_eeprom_base = eeprom_base;
    s_max_timers = max_timers;
    s_timers = (timer_config_t *)calloc(max_timers, sizeof(timer_config_t));
    s_last_fired_minute = (uint16_t *)calloc(max_timers, sizeof(uint16_t));
    s_last_fired_epoch = (unsigned long *)calloc(max_timers, sizeof(unsigned long));
    if (!s_timers || !s_last_fired_minute || !s_last_fired_epoch) return false;
    memset(s_timers, 0, max_timers * sizeof(timer_config_t));
    memset(s_last_fired_minute, 0xFF, max_timers * sizeof(uint16_t));
    memset(s_last_fired_epoch, 0, max_timers * sizeof(unsigned long));
    timer_load();
    s_loaded = true;
    return true;
}

void timer_load() {
    if (!timer_load_littlefs()) {
        memset(s_timers, 0, s_max_timers * sizeof(timer_config_t));
    }
}

void timer_save() {
    timer_save_littlefs();
}

bool timer_get(int index, timer_config_t *out) {
    if (index < 0 || index >= s_max_timers) return false;
    if (out) *out = s_timers[index];
    return true;
}

bool timer_set(int index, const timer_config_t *cfg) {
    if (index < 0 || index >= s_max_timers || !cfg) return false;
    s_timers[index] = *cfg;
    s_last_fired_minute[index] = 0xFF;
    s_last_fired_epoch[index] = 0;
    return true;
}

void timer_reset_fired(int index) {
    if (index < 0 || index >= s_max_timers) return;
    s_last_fired_minute[index] = 0xFF;
    s_last_fired_epoch[index] = 0;
}

int8_t timer_check(unsigned long current_epoch, int timezone_offset) {
    if (!s_loaded) return -1;
    if (current_epoch < 100000) return -1;
    time_t lt_epoch = (time_t)(current_epoch + (timezone_offset * 3600));
    struct tm *lt = localtime(&lt_epoch);
    uint8_t now_hour = lt->tm_hour;
    uint8_t now_min = lt->tm_min;
    uint8_t now_wday = lt->tm_wday;
    uint16_t now_minute_id = now_hour * 60 + now_min;
    for (int i = 0; i < s_max_timers; i++) {
        if (!s_timers[i].enabled) continue;
        uint16_t timer_minute_id = s_timers[i].hour * 60 + s_timers[i].minute;
        if (timer_minute_id != now_minute_id) continue;
        if (s_timers[i].days_mask != 0) {
            if (!(s_timers[i].days_mask & (1 << now_wday))) continue;
        }
        /* Anti-re-fire: só bloqueia se disparou NA MESMA MINUTOS (evita
           re-fire dentro do mesmo minuto). Limite de 120s garante que no
           dia seguinte no mesmo horário o timer possa disparar novamente. */
        if (s_last_fired_minute[i] == now_minute_id &&
            (current_epoch - s_last_fired_epoch[i]) < 120) continue;
        s_last_fired_minute[i] = now_minute_id;
        s_last_fired_epoch[i] = current_epoch;
        return (int8_t)s_timers[i].action;
    }
    return -1;
}

void timer_get_next(unsigned long current_epoch, int timezone_offset,
                    unsigned long *next_epoch, uint8_t *next_action) {
    *next_epoch = 0; *next_action = 0;
    if (!s_loaded || current_epoch < 100000) return;
    time_t lt_epoch = (time_t)(current_epoch + (timezone_offset * 3600));
    struct tm *lt = localtime(&lt_epoch);
    uint8_t now_hour = lt->tm_hour;
    uint8_t now_min = lt->tm_min;
    uint8_t now_wday = lt->tm_wday;
    uint16_t now_slot = now_hour * 60 + now_min;
    unsigned long best_epoch = 0; uint8_t best_action = 0;
    for (int i = 0; i < s_max_timers; i++) {
        if (!s_timers[i].enabled) continue;
        uint16_t timer_slot = s_timers[i].hour * 60 + s_timers[i].minute;
        bool day_ok = (s_timers[i].days_mask == 0);
        if (!day_ok) {
            for (int d = 0; d < 7; d++)
                if (s_timers[i].days_mask & (1 << d)) { day_ok = true; break; }
        }
        if (!day_ok) continue;
        unsigned long candidate = current_epoch;
        if (timer_slot <= now_slot) candidate += 86400;
        candidate += (timer_slot - now_slot) * 60;
        candidate -= timezone_offset * 3600;
        if (best_epoch == 0 || candidate < best_epoch) {
            best_epoch = candidate; best_action = s_timers[i].action;
        }
    }
    *next_epoch = best_epoch; *next_action = best_action;
}

void timer_to_json(JsonDocument &doc) {
    JsonArray arr = doc["timers"].to<JsonArray>();
    for (int i = 0; i < s_max_timers; i++) {
        JsonObject t = arr.add<JsonObject>();
        t["hour"] = s_timers[i].hour;
        t["minute"] = s_timers[i].minute;
        t["action"] = s_timers[i].action;
        t["days_mask"] = s_timers[i].days_mask;
        t["enabled"] = s_timers[i].enabled;
    }
}

bool timer_from_json(JsonDocument &doc) {
    if (!doc.containsKey("timers")) return false;
    JsonArray arr = doc["timers"].as<JsonArray>();
    int count = arr.size();
    if (count > s_max_timers) count = s_max_timers;
    for (int i = 0; i < count; i++) {
        JsonObject t = arr[i];
        s_timers[i].hour = t["hour"] | 0;
        s_timers[i].minute = t["minute"] | 0;
        s_timers[i].action = t["action"] | 0;
        s_timers[i].days_mask = t["days_mask"] | 0;
            s_timers[i].enabled = (t["enabled"] | 0) != 0;
    }
    return true;
}

// --- LittleFS persistence ---

bool timer_load_littlefs(void) {
    if (!LittleFS.exists(TIMER_LITTLEFS_FILE)) return false;
    File f = LittleFS.open(TIMER_LITTLEFS_FILE, "r");
    if (!f) return false;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;
    if (doc.containsKey("timers")) {
        JsonArray arr = doc["timers"].as<JsonArray>();
        int count = arr.size();
        if (count > s_max_timers) count = s_max_timers;
        for (int i = 0; i < count; i++) {
            JsonObject t = arr[i];
            s_timers[i].hour = t["hour"] | 0;
            s_timers[i].minute = t["minute"] | 0;
            s_timers[i].action = t["action"] | 0;
            s_timers[i].days_mask = t["days_mask"] | 0;
            s_timers[i].enabled = (t["enabled"] | 0) != 0;
        }
    }
    if (doc.containsKey("cyclic")) {
        JsonObject c = doc["cyclic"];
        s_cyclic.enabled = c["enabled"] | 0;
        s_cyclic.duration_min = c["duration_min"] | 30;
    }
    if (doc.containsKey("pulse")) {
        JsonObject p = doc["pulse"];
        s_pulse_cfg.enabled = p["enabled"] | false;
        s_pulse_cfg.duration_min = p["duration_min"] | 60;
    }
    return true;
}

bool timer_save_littlefs(void) {
    JsonDocument doc;
    JsonArray arr = doc["timers"].to<JsonArray>();
    for (int i = 0; i < s_max_timers; i++) {
        JsonObject t = arr.add<JsonObject>();
        t["hour"] = s_timers[i].hour;
        t["minute"] = s_timers[i].minute;
        t["action"] = s_timers[i].action;
        t["days_mask"] = s_timers[i].days_mask;
        t["enabled"] = s_timers[i].enabled;
    }
    JsonObject c = doc["cyclic"].to<JsonObject>();
    c["enabled"] = s_cyclic.enabled;
    c["duration_min"] = s_cyclic.duration_min;
    JsonObject p = doc["pulse"].to<JsonObject>();
    p["enabled"] = s_pulse_cfg.enabled;
    p["duration_min"] = s_pulse_cfg.duration_min;
    File f = LittleFS.open(TIMER_LITTLEFS_FILE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// --- Cyclic timer ---

int8_t cyclic_check(unsigned long now_ms, bool current_relay_state) {
    if (!s_cyclic.enabled) return 0;
    if (!s_cyclic_waiting_off) {
        if (now_ms >= s_cyclic_next_event) {
            s_cyclic_waiting_off = true;
            s_cyclic_next_event = now_ms + ((unsigned long)s_cyclic.duration_min * 60000UL);
            return 1;
        }
    } else {
        if (now_ms >= s_cyclic_next_event) {
            s_cyclic_waiting_off = false;
            s_cyclic_next_event = now_ms + ((unsigned long)s_cyclic.duration_min * 60000UL);
            return -1;
        }
    }
    return 0;
}

void cyclic_reset(void) {
    s_cyclic_waiting_off = false;
    s_cyclic_next_event = 0;
}

bool cyclic_get_enabled(void) { return s_cyclic.enabled; }
uint16_t cyclic_get_duration(void) { return s_cyclic.duration_min; }
void cyclic_set_enabled(bool enabled) { s_cyclic.enabled = enabled; }
void cyclic_set_duration(uint16_t min) { s_cyclic.duration_min = min; }

// --- Pulse config (persistence + runtime) ---

static unsigned long s_pulse_start_ms = 0;
static bool s_pulse_active = false;
static bool s_pulse_needs_init = false;

bool     timer_pulse_get_enabled(void) { return s_pulse_cfg.enabled; }
uint16_t timer_pulse_get_duration(void) { return s_pulse_cfg.duration_min; }
void     timer_pulse_set_enabled(bool enabled) { s_pulse_cfg.enabled = enabled; }
void     timer_pulse_set_duration(uint16_t min) { s_pulse_cfg.duration_min = min; }

void pulse_start(void) {
    if (!s_pulse_cfg.enabled || s_pulse_cfg.duration_min == 0) return;
    s_pulse_active = true;
    s_pulse_needs_init = true;
}

void pulse_cancel(void) {
    s_pulse_active = false;
    s_pulse_needs_init = false;
}

int8_t pulse_check(unsigned long now_ms) {
    if (!s_pulse_active || !s_pulse_cfg.enabled || s_pulse_cfg.duration_min == 0) return 0;
    if (s_pulse_needs_init) {
        s_pulse_start_ms = now_ms;
        s_pulse_needs_init = false;
    }
    unsigned long elapsed = now_ms - s_pulse_start_ms;
    unsigned long timeout = (unsigned long)s_pulse_cfg.duration_min * 60000UL;
    if (elapsed >= timeout) {
        s_pulse_active = false;
        return -1;
    }
    return 0;
}
