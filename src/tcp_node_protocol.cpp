// shared/src/tcp_node_protocol.cpp
// TCP/HTTP node protocol implementation — communicates with hub via
// HTTP REST + UDP discovery, same pattern as nodes/tcp but reusable
// by any node type (lamp, presence, etc.).

#ifdef TCP_ENABLED

#include "tcp_node_protocol.h"
#include "espnow_protocol.h"
#include "shared_config.h"
#include "common_console.h"
#include <Arduino.h>
#ifdef ESP32
  #include <HTTPClient.h>
#else
  #include <ESP8266HTTPClient.h>
#endif
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>

#define HTTP_TIMEOUT_MS 3000
#define DISCOVER_INTERVAL_MS 10000
#define REREGISTER_INTERVAL_MS 120000

// Watchdog timeouts (TCP nodes depend on hub + WiFi)
#define TCP_WIFI_WATCHDOG_MS   60000   // 1 min without WiFi → restart
#define TCP_HUB_WATCHDOG_MS   180000   // 3 min without hub → restart

static const char* TAG = "tcp-node";

TcpNodeProtocol::TcpNodeProtocol()
    : m_slot(0), m_registered(false), m_hub_found(false), m_retry_count(0)
    , m_hub_port(TCP_HTTP_PORT)
    , m_pair_interval_ms(5000), m_heartbeat_interval_ms(60000)
    , m_state_interval_ms(60000), m_last_state_ms(0)
    , m_last_heartbeat_ms(0), m_last_discover_ms(0), m_last_register_ms(0)
    , m_last_command_check_ms(0)
    , m_tx_count(0), m_rx_count(0)
    , m_last_hub_contact_ms(0), m_last_wifi_ok_ms(0)
{
    memset(m_mac, 0, sizeof(m_mac));
    memset(m_gateway_mac, 0, sizeof(m_gateway_mac));
    memset(m_device_name, 0, sizeof(m_device_name));
    memset(m_device_id, 0, sizeof(m_device_id));
    memset(m_hub_ip, 0, sizeof(m_hub_ip));
    strncpy(m_hub_ip, HUB_IP_DEFAULT, sizeof(m_hub_ip) - 1);
}

void TcpNodeProtocol::set_device_id(const char* id) {
    strncpy(m_device_id, id, sizeof(m_device_id) - 1);
    m_device_id[sizeof(m_device_id) - 1] = '\0';
}

void TcpNodeProtocol::set_mac(const uint8_t* mac) {
    memcpy(m_mac, mac, 6);
}

void TcpNodeProtocol::set_device_name(const char* name) {
    strncpy(m_device_name, name, sizeof(m_device_name) - 1);
    m_device_name[sizeof(m_device_name) - 1] = '\0';
}

void TcpNodeProtocol::set_gateway_mac(const uint8_t* mac) {
    memcpy(m_gateway_mac, mac, 6);
}

void TcpNodeProtocol::save_gateway_mac() {
    // TCP mode: hub IP is stored, not MAC. No-op for compatibility.
}

void TcpNodeProtocol::load_gateway_mac() {
    // TCP mode: hub discovered via UDP. No-op for compatibility.
}

void TcpNodeProtocol::begin() {
    m_udp.begin(TCP_UDP_PORT);
    m_last_discover_ms = 0; // trigger immediate discover
    m_last_wifi_ok_ms = millis(); // start WiFi watchdog from boot
    console.printf("[%s] Initialized (TCP mode)\n", TAG);
}

bool TcpNodeProtocol::send_to_hub(const char* endpoint, const String& payload) {
    if (WiFi.status() != WL_CONNECTED) {
         console.printf("WIFI Desconectado");
        return false;
    }
    WiFiClient client;
    HTTPClient http;
    String url = String("http://") + m_hub_ip + ":" + String(m_hub_port) + endpoint;
    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(HTTP_TIMEOUT_MS);
        int httpCode = http.POST(payload);
        if (httpCode > 0) {
            m_tx_count++;
            if (httpCode == 200) m_rx_count++;
            http.end();
            return httpCode == 200;
        }
        http.end();
    }
    return false;
}

void TcpNodeProtocol::send_udp_discover() {
    tcp_gw_discover_t disc;
    memset(&disc, 0, sizeof(disc));
    disc.msg_type = MSG_GW_DISCOVER;
    disc.sensor_type = callbacks.get_sensor_type ? callbacks.get_sensor_type() : SENSOR_TYPE_ONOFF;
    strncpy(disc.device_name, m_device_name, sizeof(disc.device_name) - 1);
    IPAddress bcast(255, 255, 255, 255);
    m_udp.beginPacket(bcast, TCP_UDP_PORT);
    m_udp.write((uint8_t*)&disc, sizeof(disc));
    m_udp.endPacket();
    m_tx_count++;
}

void TcpNodeProtocol::handle_udp_announce() {
    int sz = m_udp.parsePacket();
    if (sz <= 0) return;
    uint8_t buf[64];
    int len = m_udp.read(buf, sizeof(buf));
    if (len < (int)sizeof(tcp_gw_announce_t)) return;
    if (buf[0] != MSG_GW_ANNOUNCE) return;
    tcp_gw_announce_t* ann = (tcp_gw_announce_t*)buf;
    memset(m_hub_ip, 0, sizeof(m_hub_ip));
    strncpy(m_hub_ip, ann->hub_ip, sizeof(m_hub_ip) - 1);
    m_hub_port = ann->hub_port;
    m_hub_found = true;
    m_rx_count++;
    console.printf("[%s] Hub found: %s:%d\n", TAG, m_hub_ip, m_hub_port);
}

bool TcpNodeProtocol::register_with_hub() {
    JsonDocument doc;
    doc["device_id"] = m_device_id;
    doc["sensor_type"] = (int)(callbacks.get_sensor_type ? callbacks.get_sensor_type() : SENSOR_TYPE_ONOFF);
    doc["device_name"] = m_device_name;
    doc["fw_version"] = FW_VERSION;
    char mac_str[18];
    mac_to_str(m_mac, mac_str, sizeof(mac_str));
    doc["mac"] = mac_str;
    String payload;
    serializeJson(doc, payload);
    if (send_to_hub("/node/register", payload)) {
        m_registered = true;
        m_retry_count = 0;
        m_last_hub_contact_ms = millis();
        console.printf("[%s] Registered with hub\n", TAG);
        return true;
    }
    m_retry_count++;
    return false;
}

void TcpNodeProtocol::check_commands() {
    if (!m_registered || WiFi.status() != WL_CONNECTED) return;
    unsigned long now = millis();
    if (now - m_last_command_check_ms < 1000) return;
    m_last_command_check_ms = now;
    WiFiClient client;
    HTTPClient http;
    String url = String("http://") + m_hub_ip + ":" + String(m_hub_port) + "/node/command/" + m_device_id;
    if (http.begin(client, url)) {
        http.setTimeout(HTTP_TIMEOUT_MS);
        int httpCode = http.GET();
        if (httpCode == 200) {
            m_rx_count++;
            String response = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, response);
            if (!err && doc.containsKey("command") && !doc["command"].isNull()) {
                const char* cmd = doc["command"];
                if (strcmp(cmd, "restart") == 0) {
                    if (callbacks.on_restart) callbacks.on_restart();
                } else if (callbacks.on_command) {
                    uint8_t val = (strcmp(cmd, "on") == 0) ? 0x01 : 0x00;
                    callbacks.on_command(val);
                }
            }
        }
        http.end();
    }
}

void TcpNodeProtocol::loop() {
    unsigned long now = millis();

    // ── Watchdogs ──
    // WiFi: TCP node without WiFi is dead — restart
    if (WiFi.status() == WL_CONNECTED) {
        m_last_wifi_ok_ms = now;
    } else if (m_last_wifi_ok_ms > 0 && (now - m_last_wifi_ok_ms) > TCP_WIFI_WATCHDOG_MS) {
        console.printf("[%s] WiFi offline for %lus, restarting...\n", TAG, (now - m_last_wifi_ok_ms) / 1000);
        delay(100);
        ESP.restart();
    }
    // Hub: no contact for too long — restart
    unsigned long last_contact = m_last_hub_contact_ms > 0 ? m_last_hub_contact_ms : millis(); // start from now if never contacted
    if (m_last_hub_contact_ms > 0 && (now - m_last_hub_contact_ms) > TCP_HUB_WATCHDOG_MS) {
        console.printf("[%s] No hub contact for %lus, restarting...\n", TAG, (now - m_last_hub_contact_ms) / 1000);
        delay(100);
        ESP.restart();
    }

    // UDP discovery
    handle_udp_announce();
    if (!m_hub_found && now - m_last_discover_ms > DISCOVER_INTERVAL_MS) {
        m_last_discover_ms = now;
        send_udp_discover();
    }

    // Register if hub known but not registered
    if (m_hub_found && !m_registered) {
        if (register_with_hub()) {
            // Publish initial state (incl. IP) right after registering so the
            // hub has the node's IP and current state without waiting for the
            // first periodic interval.
            publish_state();
        }
    }

    // Periodic re-registration (handles hub reboot that regenerates bridge_device_id)
    if (m_registered && m_hub_found && now - m_last_register_ms > REREGISTER_INTERVAL_MS) {
        m_last_register_ms = now;
        register_with_hub();
    }

    // Periodic state + heartbeat
    if (m_registered) {
        if (now - m_last_state_ms > m_state_interval_ms) {
            m_last_state_ms = now;
            publish_state();
        }
        if (now - m_last_heartbeat_ms > m_heartbeat_interval_ms) {
            m_last_heartbeat_ms = now;
            JsonDocument doc;
            doc["device_id"] = m_device_id;
            String payload;
            serializeJson(doc, payload);
            send_to_hub("/node/heartbeat", payload);
            m_last_hub_contact_ms = millis();
        }
    }

    // Poll commands
    check_commands();
}

void TcpNodeProtocol::publish_state() {
    if (!m_registered || WiFi.status() != WL_CONNECTED) 
    {
        console.printf("WIFI %s", WiFi.status() != WL_CONNECTED ? "Desconectado" : "Nao registrado");
        return;
    }   

    JsonDocument doc;
    doc["device_id"] = m_device_id;
    doc["device_name"] = m_device_name;
    doc["fw_version"] = FW_VERSION;
    doc["uptime_s"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["tx_count"] = m_tx_count;
    doc["rx_count"] = m_rx_count;
    doc["ip"] = WiFi.localIP().toString();
    doc["hub_ip"] = m_hub_ip;
    doc["slot"] = m_slot;

    // Append sensor payload via callback
    if (callbacks.get_sensor_payload) {
        uint8_t buf[32];
        uint8_t len = callbacks.get_sensor_payload(buf, sizeof(buf));
        // For ONOFF/LIGHT: extract relay state
        if (len >= sizeof(payload_onoff_t)) {
            payload_onoff_t* pl = (payload_onoff_t*)buf;
            doc["relay_state"] = (pl->state != 0);
            doc["state"] = (pl->state != 0);
        }
    }

    String payload;
    serializeJson(doc, payload);
    if (send_to_hub("/node/state", payload)) {
        m_last_state_ms = millis();
        m_last_hub_contact_ms = millis();
    }
}

void TcpNodeProtocol::force_repair() {
    m_registered = false;
    m_hub_found = false;
    m_last_discover_ms = 0;
    console.printf("[%s] Force repair: will re-discover hub\n", TAG);
}

#endif // TCP_ENABLED
