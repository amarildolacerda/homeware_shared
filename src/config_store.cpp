#include "config_store.h"
#include "shared_config.h"
#include "common_console.h"
#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// Migration marker in EEPROM (offset 500, outside all layouts)
#define EEPROM_MIGRATION_MARKER 500
#define EEPROM_MIGRATION_DONE   0xAA

// --- Init ---

void config_store_init() {
    if (!LittleFS.begin()) {
        console.println("[CFG] LittleFS mount failed, formatting...");
        LittleFS.format();
        LittleFS.begin();
    }
    console.println("[CFG] LittleFS mounted");

    // Check if migration needed
    EEPROM.begin(512);
    uint8_t marker = EEPROM.read(EEPROM_MIGRATION_MARKER);
    EEPROM.end();
    if (marker != EEPROM_MIGRATION_DONE) {
        console.println("[CFG] First boot — migrating EEPROM → LittleFS...");
        config_migrate_from_eeprom();
    }
}

// --- Generic helpers ---

bool config_file_exists(const char *path) {
    return LittleFS.exists(path);
}

bool config_file_remove(const char *path) {
    return LittleFS.remove(path);
}

// --- WiFi Config ---

bool config_wifi_load(WiFiConfig *cfg) {
    memset(cfg, 0, sizeof(WiFiConfig));
    cfg->channel = 0; // auto

    if (!LittleFS.exists(CFG_FILE_WIFI)) return false;

    File f = LittleFS.open(CFG_FILE_WIFI, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    strncpy(cfg->ssid,     doc["ssid"]     | "", sizeof(cfg->ssid) - 1);
    strncpy(cfg->password, doc["password"]  | "", sizeof(cfg->password) - 1);
    cfg->mode   = doc["mode"]   | 0;
    strncpy(cfg->ip,      doc["ip"]      | "", sizeof(cfg->ip) - 1);
    strncpy(cfg->gateway, doc["gateway"]  | "", sizeof(cfg->gateway) - 1);
    strncpy(cfg->netmask, doc["netmask"]  | "", sizeof(cfg->netmask) - 1);
    strncpy(cfg->dns,     doc["dns"]      | "", sizeof(cfg->dns) - 1);
    cfg->channel = doc["channel"] | 0;
    if (cfg->channel > 13) cfg->channel = 0;

    return true;
}

bool config_wifi_save(const WiFiConfig *cfg) {
    JsonDocument doc;
    doc["ssid"]     = cfg->ssid;
    doc["password"] = cfg->password;
    doc["mode"]     = cfg->mode;
    doc["ip"]       = cfg->ip;
    doc["gateway"]  = cfg->gateway;
    doc["netmask"]  = cfg->netmask;
    doc["dns"]      = cfg->dns;
    doc["channel"]  = cfg->channel;

    File f = LittleFS.open(CFG_FILE_WIFI, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// --- Device Config ---

bool config_device_load(DeviceConfig *cfg) {
    memset(cfg, 0, sizeof(DeviceConfig));

    if (!LittleFS.exists(CFG_FILE_DEVICE)) return false;

    File f = LittleFS.open(CFG_FILE_DEVICE, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    strncpy(cfg->name, doc["name"] | "", sizeof(cfg->name) - 1);

    if (doc.containsKey("gateway_mac") && doc["gateway_mac"].is<JsonArray>()) {
        JsonArray mac = doc["gateway_mac"];
        for (int i = 0; i < 6 && i < (int)mac.size(); i++) {
            cfg->gateway_mac[i] = mac[i] | 0;
        }
        cfg->gateway_mac_valid = true;
    }

    return true;
}

bool config_device_save(const DeviceConfig *cfg) {
    JsonDocument doc;
    doc["name"] = cfg->name;

    if (cfg->gateway_mac_valid) {
        JsonArray mac = doc["gateway_mac"].to<JsonArray>();
        for (int i = 0; i < 6; i++) {
            mac.add(cfg->gateway_mac[i]);
        }
    }

    File f = LittleFS.open(CFG_FILE_DEVICE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// --- Hub Config ---

static void _load_sensor(JsonObject &obj, SensorSlot *s) {
    s->paired      = obj["paired"] | false;
    s->sensor_type = obj["type"]   | 0;
    s->slot        = obj["slot"]   | 0;
    s->client_chip = obj["chip"]   | 0xFF;
    s->radio_type  = obj["radio"]  | 0;

    const char *mac = obj["mac"] | "";
    if (strlen(mac) >= 17) {
        unsigned int m[6];
        if (sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                   &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6) {
            for (int i = 0; i < 6; i++) s->mac[i] = (uint8_t)m[i];
        }
    }

    strncpy(s->name, obj["name"] | "", sizeof(s->name) - 1);
    strncpy(s->bridge_device_id, obj["bridge_id"] | "", sizeof(s->bridge_device_id) - 1);
}

static void _save_sensor(JsonObject &obj, const SensorSlot *s) {
    obj["paired"]         = s->paired;
    obj["type"]           = s->sensor_type;
    obj["slot"]           = s->slot;
    obj["chip"]           = s->client_chip;
    obj["radio"]          = s->radio_type;
    obj["name"]           = s->name;
    obj["bridge_id"]      = s->bridge_device_id;

    char mac_buf[18];
    snprintf(mac_buf, sizeof(mac_buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             s->mac[0], s->mac[1], s->mac[2], s->mac[3], s->mac[4], s->mac[5]);
    obj["mac"] = mac_buf;
}

bool config_hub_load(HubConfig *cfg) {
    memset(cfg, 0, sizeof(HubConfig));
    cfg->mqtt.port = 1883;

    if (!LittleFS.exists(CFG_FILE_HUB)) return false;

    File f = LittleFS.open(CFG_FILE_HUB, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    // MQTT
    if (doc.containsKey("mqtt")) {
        JsonObject m = doc["mqtt"];
        strncpy(cfg->mqtt.host,     m["host"]     | "", sizeof(cfg->mqtt.host) - 1);
        cfg->mqtt.port              = m["port"]     | 1883;
        strncpy(cfg->mqtt.user,     m["user"]      | "", sizeof(cfg->mqtt.user) - 1);
        strncpy(cfg->mqtt.password, m["password"]  | "", sizeof(cfg->mqtt.password) - 1);
    }

    cfg->pairing_enabled = doc["pairing"] | false;
    cfg->op_mode         = doc["op_mode"] | 0;

    // Sensors
    if (doc.containsKey("sensors") && doc["sensors"].is<JsonArray>()) {
        JsonArray arr = doc["sensors"];
        int count = arr.size();
        if (count > 64) count = 64;
        for (int i = 0; i < count; i++) {
            JsonObject obj = arr[i];
            _load_sensor(obj, &cfg->sensors[i]);
        }
    }

    return true;
}

bool config_hub_save(const HubConfig *cfg) {
    JsonDocument doc;

    // MQTT
    JsonObject m = doc["mqtt"].to<JsonObject>();
    m["host"]     = cfg->mqtt.host;
    m["port"]     = cfg->mqtt.port;
    m["user"]     = cfg->mqtt.user;
    m["password"] = cfg->mqtt.password;

    doc["pairing"] = cfg->pairing_enabled;
    doc["op_mode"] = cfg->op_mode;

    // Sensors
    JsonArray arr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < 64; i++) {
        if (cfg->sensors[i].paired) {
            JsonObject obj = arr.add<JsonObject>();
            _save_sensor(obj, &cfg->sensors[i]);
        }
    }

    File f = LittleFS.open(CFG_FILE_HUB, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// --- Node Config ---

bool config_node_load(NodeConfig *cfg) {
    memset(cfg, 0, sizeof(NodeConfig));
    cfg->relay_pin = 255;
    cfg->button_pin = 255;
    cfg->startup_mode = 0;
    cfg->sleep_interval = 300;

    if (!LittleFS.exists(CFG_FILE_NODE)) return false;

    File f = LittleFS.open(CFG_FILE_NODE, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    // Relay
    if (doc.containsKey("relay")) {
        JsonObject r = doc["relay"];
        cfg->relay_state  = r["state"]    | false;
        cfg->relay_pin    = r["pin"]      | 255;
        cfg->button_pin   = r["button"]   | 255;
        cfg->led_enabled  = r["led"]      | true;
        cfg->startup_mode = r["startup"]  | 0;
    }

    cfg->repeater_enabled = doc["repeater"] | false;
    cfg->op_mode          = doc["op_mode"]  | 0;

    // Enable flags
    if (doc.containsKey("enable")) {
        JsonObject e = doc["enable"];
        cfg->temp_enabled = e["temp"] | true;
        cfg->gas_enabled  = e["gas"]  | true;
    }

    cfg->sleep_interval = doc["sleep_interval"] | 300;

    // Timers
    if (doc.containsKey("timers") && doc["timers"].is<JsonArray>()) {
        JsonArray arr = doc["timers"];
        int count = arr.size();
        if (count > CFG_MAX_TIMERS) count = CFG_MAX_TIMERS;
        for (int i = 0; i < count; i++) {
            JsonObject t = arr[i];
            cfg->timers[i].enabled   = t["enabled"]   | false;
            cfg->timers[i].hour      = t["hour"]      | 0;
            cfg->timers[i].minute    = t["minute"]    | 0;
            cfg->timers[i].days_mask = t["days_mask"] | 0x7F;
            cfg->timers[i].action    = t["action"]    | 0;
        }
    }

    return true;
}

bool config_node_save(const NodeConfig *cfg) {
    JsonDocument doc;

    // Relay
    JsonObject r = doc["relay"].to<JsonObject>();
    r["state"]   = cfg->relay_state;
    r["pin"]     = cfg->relay_pin;
    r["button"]  = cfg->button_pin;
    r["led"]     = cfg->led_enabled;
    r["startup"] = cfg->startup_mode;

    doc["repeater"] = cfg->repeater_enabled;
    doc["op_mode"]  = cfg->op_mode;
    doc["sleep_interval"] = cfg->sleep_interval;

    // Enable flags
    JsonObject e = doc["enable"].to<JsonObject>();
    e["temp"] = cfg->temp_enabled;
    e["gas"]  = cfg->gas_enabled;

    // Timers
    JsonArray arr = doc["timers"].to<JsonArray>();
    for (int i = 0; i < CFG_MAX_TIMERS; i++) {
        if (cfg->timers[i].enabled || cfg->timers[i].hour > 0 || cfg->timers[i].minute > 0) {
            JsonObject t = arr.add<JsonObject>();
            t["enabled"]   = cfg->timers[i].enabled;
            t["hour"]      = cfg->timers[i].hour;
            t["minute"]    = cfg->timers[i].minute;
            t["days_mask"] = cfg->timers[i].days_mask;
            t["action"]    = cfg->timers[i].action;
        }
    }

    File f = LittleFS.open(CFG_FILE_NODE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// --- EEPROM Migration ---

void config_migrate_from_eeprom() {
    EEPROM.begin(512);

    // --- WiFi ---
    WiFiConfig wifi;
    memset(&wifi, 0, sizeof(wifi));
    for (int i = 0; i < 32; i++) wifi.ssid[i]     = EEPROM.read(EEPROM_WIFI_SSID_OFFSET + i);
    for (int i = 0; i < 64; i++) wifi.password[i]  = EEPROM.read(EEPROM_WIFI_PASS_OFFSET + i);
    wifi.mode    = EEPROM.read(EEPROM_WIFI_MODE_OFFSET);
    for (int i = 0; i < 15; i++) wifi.ip[i]       = EEPROM.read(EEPROM_WIFI_IP_OFFSET + i);
    for (int i = 0; i < 15; i++) wifi.gateway[i]   = EEPROM.read(EEPROM_WIFI_GW_OFFSET + i);
    for (int i = 0; i < 15; i++) wifi.netmask[i]   = EEPROM.read(EEPROM_WIFI_MASK_OFFSET + i);
    for (int i = 0; i < 15; i++) wifi.dns[i]       = EEPROM.read(EEPROM_WIFI_DNS_OFFSET + i);
    wifi.channel = EEPROM.read(EEPROM_WIFI_CHANNEL_OFFSET);

    // Validate: if SSID is all zeros or garbage, skip
    bool wifi_valid = false;
    for (int i = 0; i < 32; i++) {
        if (wifi.ssid[i] != 0 && wifi.ssid[i] != 0xFF) { wifi_valid = true; break; }
    }
    if (wifi_valid) {
        config_wifi_save(&wifi);
        console.printf("[CFG] Migrated WiFi: %s\n", wifi.ssid);
    }

    // --- Device name (from ESP-NOW client area) ---
    DeviceConfig dev;
    memset(&dev, 0, sizeof(dev));
    uint8_t marker = EEPROM.read(10);
    if (marker == 0xFF) {
        for (int i = 0; i < 47; i++) dev.name[i] = EEPROM.read(11 + i);
        dev.gateway_mac_valid = false;
        config_device_save(&dev);
        console.printf("[CFG] Migrated device name: %s\n", dev.name);
    }

    EEPROM.end();

    // Mark migration done
    EEPROM.begin(512);
    EEPROM.write(EEPROM_MIGRATION_MARKER, EEPROM_MIGRATION_DONE);
    EEPROM.commit();
    EEPROM.end();

    console.println("[CFG] EEPROM migration complete");
}
