#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

// ============================================================================
// Hybrid ConfigStore: LittleFS for complex/rarely-written data, EEPROM for
// simple/frequently-accessed data.
//
// LittleFS (file):  sensor_registry (hub), timers (nodes), MQTT config (hub)
// EEPROM (kept):    WiFi creds, device name, gateway MAC, relay state,
//                   operation mode, pairing enabled, enable flags, etc.
// ============================================================================

// --- LittleFS file paths ---
#define CFG_FILE_SENSORS  "/sensors.json"
#define CFG_FILE_MQTT     "/mqtt.json"
#define CFG_FILE_TIMERS   "/timers.json"

// --- Sensor Registry Slot (hub — stored in LittleFS) ---
struct SensorSlot {
    bool     paired;
    uint8_t  sensor_type;
    uint8_t  slot;
    uint8_t  mac[6];
    char     name[32];
    uint8_t  client_chip;
    uint8_t  radio_type;
    char     bridge_device_id[16];
};

// --- MQTT Config (hub — stored in LittleFS) ---
struct MQTTConfig {
    char     host[64];
    uint16_t port;
    char     user[32];
    char     password[32];
};

// --- Timer Config (nodes — stored in LittleFS) ---
struct TimerCfg {
    bool     enabled;
    uint8_t  hour;
    uint8_t  minute;
    uint16_t days_mask;    // bit0=Mon..bit6=Sun
    uint8_t  action;       // 0=OFF, 1=ON
};

#define CFG_MAX_TIMERS 6

// --- API ---

// Initialize LittleFS (called once in setup)
void config_store_init();

// Hub: sensor registry (64 slots in /sensors.json)
bool config_sensors_load(SensorSlot sensors[], int max_slots);
bool config_sensors_save(const SensorSlot sensors[], int count);

// Hub: MQTT config (/mqtt.json)
bool config_mqtt_load(MQTTConfig *cfg);
bool config_mqtt_save(const MQTTConfig *cfg);

// Node: timers (/timers.json)
bool config_timers_load(TimerCfg timers[], int max_timers);
bool config_timers_save(const TimerCfg timers[], int count);

// Generic helpers
bool config_file_exists(const char *path);
bool config_file_remove(const char *path);

#endif
