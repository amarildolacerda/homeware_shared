#include "espnow_node_protocol.h"
#include "espnow_protocol.h"
#include "common_espnow.h"
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <string.h>

static EspnowNodeProtocol* s_self = nullptr;

#if defined(ARDUINO_ARCH_ESP32)
extern "C" void espnow_recv_cb(const uint8_t* mac, const uint8_t* data, int len) {
    if (s_self) s_self->handle_frame(mac, data, (size_t)len);
}

extern "C" void espnow_send_cb(const uint8_t* mac, esp_now_send_status_t status) {
    if (s_self) s_self->on_send_done(mac, (uint8_t)status);
}
#else
extern "C" void espnow_recv_cb(uint8_t* mac, uint8_t* data, uint8_t len) {
    if (s_self) s_self->handle_frame(mac, data, len);
}

extern "C" void espnow_send_cb(uint8_t* mac, uint8_t status) {
    if (s_self) s_self->on_send_done(mac, status);
}
#endif

EspnowNodeProtocol::EspnowNodeProtocol()
    : m_paired(false), m_slot(0), m_sequence(0), m_espnow_ready(false)
    , m_ack_received(false), m_retries_left(0)
    , m_pair_interval_ms(5000), m_heartbeat_interval_ms(60000)
    , m_state_interval_ms(60000), m_last_pair_ms(0)
    , m_last_heartbeat_ms(0), m_last_state_ms(0), m_send_deadline(0)
    , m_retry_delay_ms(0), m_pair_attempts(0), m_pair_attempts_max(20)
    , m_last_rssi(0), m_tx_count(0), m_rx_count(0), m_last_send_sequence(0)
    , m_send_state(SEND_IDLE)
{
    memset(m_mac, 0, 6);
    memset(m_gateway_mac, 0, 6);
    memset(m_device_name, 0, sizeof(m_device_name));
    memset(m_fw_version, 0, sizeof(m_fw_version));
}

void EspnowNodeProtocol::set_mac(const uint8_t* mac) {
    memcpy(m_mac, mac, 6);
}

void EspnowNodeProtocol::set_device_name(const char* name) {
    strncpy(m_device_name, name, sizeof(m_device_name) - 1);
    m_device_name[sizeof(m_device_name) - 1] = '\0';
}

void EspnowNodeProtocol::load_gateway_mac() {
    m_paired = espnow_load_gateway_mac(m_gateway_mac, "node");
}

void EspnowNodeProtocol::save_gateway_mac() {
    espnow_save_gateway_mac(m_gateway_mac, "node");
}

void EspnowNodeProtocol::begin() {
    s_self = this;
    m_espnow_ready = espnow_client_init("node");
    if (m_espnow_ready) {
        esp_now_register_send_cb(espnow_send_cb);
        esp_now_register_recv_cb(espnow_recv_cb);
        /* Adicionar peer broadcast uma única vez — evita del+add em cada envio */
        uint8_t bc[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        espnow_client_add_peer(bc, "node");
    }
    m_paired = false;
    m_pair_attempts = 0;
    m_last_pair_ms = 0;
    m_last_heartbeat_ms = 0;
    m_last_state_ms = 0;
    m_ack_received = false;
    m_retries_left = 0;
    m_sequence = 0;
    m_send_state = SEND_IDLE;
    send_pair_request();
}

void EspnowNodeProtocol::force_repair() {
    m_paired = false;
    m_pair_attempts = 0;
    m_last_pair_ms = 0;
    m_send_state = SEND_IDLE;
}

void EspnowNodeProtocol::publish_state() {
    if (!m_paired) return;
    m_last_state_ms = 0;
}

void EspnowNodeProtocol::loop() {
    if (!m_espnow_ready) return;
    unsigned long now = millis();

    if (!m_paired) {
        if (now - m_last_pair_ms >= m_pair_interval_ms &&
            m_pair_attempts < m_pair_attempts_max) {
            m_last_pair_ms = now;
            m_pair_attempts++;
            send_pair_request();
        } else if (m_pair_attempts >= m_pair_attempts_max && callbacks.on_pairing_failed) {
            /* All attempts on current AP exhausted — notify caller to try next AP */
            m_pair_attempts = 0;
            m_last_pair_ms = 0;
            callbacks.on_pairing_failed();
        }
        return;
    }

    switch (m_send_state) {
    case SEND_IDLE:
        if (now - m_last_state_ms >= m_state_interval_ms) {
            m_last_state_ms = now;
            send_sensor_data();
            m_send_state = SEND_WAIT_ACK;
            m_send_deadline = now + 300;
            m_retries_left = 3;
            m_ack_received = false;
        }
        if (now - m_last_heartbeat_ms >= m_heartbeat_interval_ms) {
            m_last_heartbeat_ms = now;
            send_heartbeat();
        }
        break;

    case SEND_WAIT_ACK:
        if (m_ack_received) {
            m_send_state = SEND_IDLE;
        } else if (now >= m_send_deadline) {
            m_retries_left--;
            if (m_retries_left > 0) {
                m_send_state = SEND_RETRY_DELAY;
                m_retry_delay_ms = now + 50;
            } else {
                force_repair();
            }
        }
        break;

    case SEND_RETRY_DELAY:
        if (now >= m_retry_delay_ms) {
            send_sensor_data();
            m_send_state = SEND_RETRY_WAIT_ACK;
            m_send_deadline = now + 300;
            m_ack_received = false;
        }
        break;

    case SEND_RETRY_WAIT_ACK:
        if (m_ack_received) {
            m_send_state = SEND_IDLE;
        } else if (now >= m_send_deadline) {
            m_retries_left--;
            if (m_retries_left > 0) {
                m_send_state = SEND_RETRY_DELAY;
                m_retry_delay_ms = now + 50;
            } else {
                force_repair();
            }
        }
        break;
    }
}

void EspnowNodeProtocol::on_send_done(const uint8_t* mac, uint8_t status) {
    (void)mac;
    (void)status;
    m_tx_count++;
}

void EspnowNodeProtocol::send_pair_request() {
    espnow_pair_request_t req;
    memset(&req, 0, sizeof(req));
    req.msg_type = MSG_PAIR_REQUEST;
    req.sequence = m_sequence++;
    memcpy(req.sensor_mac, m_mac, 6);
    req.sensor_type = callbacks.get_sensor_type ? callbacks.get_sensor_type() : 0;
    strncpy(req.device_name, m_device_name, sizeof(req.device_name) - 1);
    req.device_name[sizeof(req.device_name) - 1] = '\0';
    uint8_t bc[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    espnow_send_wrapper(bc, (uint8_t*)&req, sizeof(req), "node");
}

void EspnowNodeProtocol::send_sensor_data() {
    uint8_t payload[ESPNOW_MAX_PAYLOAD];
    uint8_t payload_len = 0;
    if (callbacks.get_sensor_payload) {
        payload_len = callbacks.get_sensor_payload(payload, ESPNOW_MAX_PAYLOAD);
    }
    uint8_t buf[ESPNOW_HEADER_FIXED_SIZE + payload_len + 4];
    espnow_header_t* hdr = (espnow_header_t*)buf;
    hdr->version = ESPNOW_PROTOCOL_VERSION;
    hdr->msg_type = MSG_SENSOR_DATA;
    hdr->sequence = m_sequence++;
    memcpy(hdr->sensor_mac, m_mac, 6);
    hdr->sensor_type = callbacks.get_sensor_type ? callbacks.get_sensor_type() : 0;
    hdr->battery_pct = 100;
    hdr->rssi = 0;
    hdr->payload_len = payload_len;
    // Fill node WiFi IP
    {
        IPAddress ip = WiFi.localIP();
        hdr->ip[0] = ip[0]; hdr->ip[1] = ip[1]; hdr->ip[2] = ip[2]; hdr->ip[3] = ip[3];
    }
    if (payload_len > 0) memcpy(hdr->payload, payload, payload_len);
    uint8_t bc[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    espnow_send_wrapper(bc, buf, ESPNOW_HEADER_FIXED_SIZE + payload_len, "node");
    m_last_send_sequence = hdr->sequence;
}

void EspnowNodeProtocol::send_heartbeat() {
    uint8_t buf[ESPNOW_HEADER_FIXED_SIZE];
    espnow_header_t* hdr = (espnow_header_t*)buf;
    hdr->version = ESPNOW_PROTOCOL_VERSION;
    hdr->msg_type = MSG_HEARTBEAT;
    hdr->sequence = m_sequence++;
    memcpy(hdr->sensor_mac, m_mac, 6);
    hdr->sensor_type = 0;
    hdr->battery_pct = 100;
    hdr->rssi = 0;
    hdr->payload_len = 0;
    // Fill node WiFi IP
    {
        IPAddress ip = WiFi.localIP();
        hdr->ip[0] = ip[0]; hdr->ip[1] = ip[1]; hdr->ip[2] = ip[2]; hdr->ip[3] = ip[3];
    }
    uint8_t bc[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    espnow_send_wrapper(bc, buf, ESPNOW_HEADER_FIXED_SIZE, "node");
}

void EspnowNodeProtocol::handle_frame(const uint8_t* mac, const uint8_t* data, size_t len) {
    m_last_rssi = 0;
    m_rx_count++;
    if (len < 3) return;

    uint8_t msg_type = data[0];

    if (msg_type == MSG_PAIR_RESPONSE) {
        if (len >= sizeof(espnow_pair_response_t)) {
            const espnow_pair_response_t* resp = (const espnow_pair_response_t*)data;
            if (memcmp(resp->sensor_mac, m_mac, 6) == 0) {
                m_paired = true;
                m_slot = resp->assigned_slot;
                memcpy(m_gateway_mac, mac, 6);
                save_gateway_mac();
                m_pair_attempts = 0;
                if (callbacks.on_paired) callbacks.on_paired(m_slot);
                m_last_state_ms = 0;
            }
        }
    } else if (msg_type == MSG_ACK) {
        if (len >= sizeof(espnow_ack_t)) {
            const espnow_ack_t* ack = (const espnow_ack_t*)data;
            if (ack->sequence == m_last_send_sequence) {
                m_ack_received = true;
                if (ack->status == PAIR_STATUS_DENIED) {
                    force_repair();
                }
            }
        }
    } else if (msg_type == MSG_NAK) {
        if (len >= sizeof(espnow_nak_t)) {
            const espnow_nak_t* nak = (const espnow_nak_t*)data;
            if (nak->reason == NAK_REASON_GATEWAY_LOST) {
                force_repair();
            }
        }
    } else if (msg_type == MSG_RESTART) {
        if (len >= sizeof(espnow_restart_t)) {
            const espnow_restart_t* rst = (const espnow_restart_t*)data;
            if (memcmp(rst->target_mac, m_mac, 6) == 0) {
                if (callbacks.on_restart) callbacks.on_restart();
            }
        }
    } else if (msg_type == MSG_COMMAND) {
        if (len >= sizeof(espnow_command_t)) {
            const espnow_command_t* cmd = (const espnow_command_t*)data;
            if (memcmp(cmd->target_mac, m_mac, 6) == 0) {
                if (callbacks.on_command) callbacks.on_command(cmd->command);
            }
        }
    } else if (msg_type == MSG_TIME_SYNC) {
        if (len >= sizeof(espnow_time_sync_t)) {
            const espnow_time_sync_t* ts = (const espnow_time_sync_t*)data;
            if (callbacks.on_time_sync) callbacks.on_time_sync(ts->epoch_seconds);
        }
    } else {
        if (callbacks.on_forward) {
            callbacks.on_forward(data, len, mac);
        }
    }
}
