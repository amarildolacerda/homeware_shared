#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

// --- File paths on LittleFS ---
#define CFG_FILE_WIFI    "/wifi.json"
#define CFG_FILE_DEVICE  "/device.json"
#define CFG_FILE_HUB     "/hub.json"
#define CFG_FILE_NODE    "/node.json"

// --- WiFi Config ---
struct WiFiConfig {
    char     ssid[33];
    char     password[65];
    uint8_t  mode;        // 0=DHCP, 1=Static
    char     ip[16];
    char     gateway[16];
    char     netmask[16];
    char     dns[16];
    uint8_t  channel;     // 0=auto, 1-13=forced
};

// --- Device Config (shared) ---
struct DeviceConfig {
    char     name[48];
    uint8_t  gateway_mac[6];
    bool     gateway_mac_valid;
};

// --- Sensor Registry Slot (hub) ---
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

// --- MQTT Config (hub) ---
struct MQTTConfig {
    char     host[64];
    uint16_t port;
    char     user[32];
    char     password[32];
};

// --- Hub Config ---
struct HubConfig {
    SensorSlot sensors[64];
    MQTTConfig mqtt;
    bool     pairing_enabled;
    uint8_t  op_mode;      // 0=Terminal, 1=AP, 2=Hybrid
};

// --- Timer Config (nodes) ---
struct TimerCfg {
    bool     enabled;
    uint8_t  hour;
    uint8_t  minute;
    uint16_t days_mask;    // bit0=Mon..bit6=Sun
    uint8_t  action;       // 0=OFF, 1=ON
};

#define CFG_MAX_TIMERS 6

// --- Node Config ---
struct NodeConfig {
    // Relay
    bool     relay_state;
    uint8_t  relay_pin;
    uint8_t  button_pin;
    bool     led_enabled;
    uint8_t  startup_mode;  // 0=OFF, 1=ON, 2=SAME
    // Repeater
    bool     repeater_enabled;
    // Timer
    TimerCfg timers[CFG_MAX_TIMERS];
    // Operation mode
    uint8_t  op_mode;
    // Climate/Gas enable flags
    bool     temp_enabled;
    bool     gas_enabled;
    // Deep sleep interval
    uint32_t sleep_interval;
};

// --- API ---

// Initialize LittleFS + auto-migrate from EEPROM
void config_store_init();

// WiFi
bool config_wifi_load(WiFiConfig *cfg);
bool config_wifi_save(const WiFiConfig *cfg);

// Device
bool config_device_load(DeviceConfig *cfg);
bool config_device_save(const DeviceConfig *cfg);

// Hub
bool config_hub_load(HubConfig *cfg);
bool config_hub_save(const HubConfig *cfg);

// Node
bool config_node_load(NodeConfig *cfg);
bool config_node_save(const NodeConfig *cfg);

// Generic helpers
bool config_file_exists(const char *path);
bool config_file_remove(const char *path);

// Migration from EEPROM (called once)
void config_migrate_from_eeprom();

#endif
