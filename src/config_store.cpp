#include "config_store.h"
#include "common_console.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// ============================================================================
// Hybrid ConfigStore — LittleFS only for complex/rarely-written data
// ============================================================================

// --- Init ---

void config_store_init() {
    if (!LittleFS.begin()) {
        console.println("[CFG] LittleFS mount failed, formatting...");
        LittleFS.format();
        LittleFS.begin();
    }
    console.println("[CFG] LittleFS mounted");
}

// --- Generic helpers ---

bool config_file_exists(const char *path) {
    return LittleFS.exists(path);
}

bool config_file_remove(const char *path) {
    return LittleFS.remove(path);
}

// ============================================================================
// Hub: Sensor Registry (/sensors.json)
// ============================================================================

static void _load_slot(JsonObject &obj, SensorSlot *s) {
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

static void _save_slot(JsonObject &obj, const SensorSlot *s) {
    obj["paired"]    = s->paired;
    obj["type"]      = s->sensor_type;
    obj["slot"]      = s->slot;
    obj["chip"]      = s->client_chip;
    obj["radio"]     = s->radio_type;
    obj["name"]      = s->name;
    obj["bridge_id"] = s->bridge_device_id;

    char mac_buf[18];
    snprintf(mac_buf, sizeof(mac_buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             s->mac[0], s->mac[1], s->mac[2], s->mac[3], s->mac[4], s->mac[5]);
    obj["mac"] = mac_buf;
}

bool config_sensors_load(SensorSlot sensors[], int max_slots) {
    memset(sensors, 0, sizeof(SensorSlot) * max_slots);

    if (!LittleFS.exists(CFG_FILE_SENSORS)) return false;

    File f = LittleFS.open(CFG_FILE_SENSORS, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }

    if (!doc["sensors"].is<JsonArray>()) {
        f.close();
        return false;
    }

    JsonArray arr = doc["sensors"];
    int count = arr.size();
    if (count > max_slots) count = max_slots;
    for (int i = 0; i < count; i++) {
        JsonObject obj = arr[i];
        _load_slot(obj, &sensors[i]);
    }
    f.close();
    return true;
}

bool config_sensors_save(const SensorSlot sensors[], int count) {
    JsonDocument doc;
    JsonArray arr = doc["sensors"].to<JsonArray>();

    for (int i = 0; i < count; i++) {
        if (sensors[i].paired) {
            JsonObject obj = arr.add<JsonObject>();
            _save_slot(obj, &sensors[i]);
        }
    }

    File f = LittleFS.open(CFG_FILE_SENSORS, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// ============================================================================
// Hub: MQTT Config (/mqtt.json)
// ============================================================================

bool config_mqtt_load(MQTTConfig *cfg) {
    memset(cfg, 0, sizeof(MQTTConfig));
    cfg->port = 1883;

    if (!LittleFS.exists(CFG_FILE_MQTT)) return false;

    File f = LittleFS.open(CFG_FILE_MQTT, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    strncpy(cfg->host,     doc["host"]     | "", sizeof(cfg->host) - 1);
    cfg->port              = doc["port"]     | 1883;
    strncpy(cfg->user,     doc["user"]      | "", sizeof(cfg->user) - 1);
    strncpy(cfg->password, doc["password"]  | "", sizeof(cfg->password) - 1);

    return true;
}

bool config_mqtt_save(const MQTTConfig *cfg) {
    JsonDocument doc;
    doc["host"]     = cfg->host;
    doc["port"]     = cfg->port;
    doc["user"]     = cfg->user;
    doc["password"] = cfg->password;

    File f = LittleFS.open(CFG_FILE_MQTT, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// ============================================================================
// Node: Timers (/timers.json)
// ============================================================================

bool config_timers_load(TimerCfg timers[], int max_timers) {
    memset(timers, 0, sizeof(TimerCfg) * max_timers);

    if (!LittleFS.exists(CFG_FILE_TIMERS)) return false;

    File f = LittleFS.open(CFG_FILE_TIMERS, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return false; }
    f.close();

    if (!doc["timers"].is<JsonArray>()) {
        return false;
    }

    JsonArray arr = doc["timers"];
    int count = arr.size();
    if (count > max_timers) count = max_timers;
    for (int i = 0; i < count; i++) {
        JsonObject t = arr[i];
        timers[i].enabled   = t["enabled"]   | false;
        timers[i].hour      = t["hour"]      | 0;
        timers[i].minute    = t["minute"]    | 0;
        timers[i].days_mask = t["days_mask"] | 0x7F;
        timers[i].action    = t["action"]    | 0;
    }

    return true;
}

bool config_timers_save(const TimerCfg timers[], int count) {
    JsonDocument doc;
    JsonArray arr = doc["timers"].to<JsonArray>();

    for (int i = 0; i < count; i++) {
        if (timers[i].enabled || timers[i].hour > 0 || timers[i].minute > 0) {
            JsonObject t = arr.add<JsonObject>();
            t["enabled"]   = timers[i].enabled;
            t["hour"]      = timers[i].hour;
            t["minute"]    = timers[i].minute;
            t["days_mask"] = timers[i].days_mask;
            t["action"]    = timers[i].action;
        }
    }

    File f = LittleFS.open(CFG_FILE_TIMERS, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}
