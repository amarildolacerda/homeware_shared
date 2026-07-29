#include "lora_node_protocol.h"
#include "lora_protocol.h"
#include <Arduino.h>
#include <string.h>

static unsigned long s_default_pair_ms = 5000;
static unsigned long s_default_heartbeat_ms = 60000;
static unsigned long s_default_state_ms = 60000;
static uint8_t s_max_pair_attempts = 20;

LoraNodeProtocol::LoraNodeProtocol(RadioInterface* radio)
    : m_radio(radio)
    , m_paired(false)
    , m_slot(0)
    , m_sequence(0)
    , m_pair_interval_ms(s_default_pair_ms)
    , m_heartbeat_interval_ms(s_default_heartbeat_ms)
    , m_state_interval_ms(s_default_state_ms)
    , m_last_pair_ms(0)
    , m_last_heartbeat_ms(0)
    , m_last_state_ms(0)
    , m_pair_attempts(0)
{
    memset(m_mac, 0, sizeof(m_mac));
    memset(m_device_name, 0, sizeof(m_device_name));
}

void LoraNodeProtocol::set_mac(const uint8_t* mac) {
    memcpy(m_mac, mac, 6);
}

void LoraNodeProtocol::set_device_name(const char* name) {
    strncpy(m_device_name, name, sizeof(m_device_name) - 1);
    m_device_name[sizeof(m_device_name) - 1] = '\0';
}

void LoraNodeProtocol::begin() {
    m_paired = false;
    m_pair_attempts = 0;
    m_last_pair_ms = 0;
    m_last_heartbeat_ms = 0;
    m_last_state_ms = 0;
    m_sequence = 0;
    m_radio->set_rx_callback(rx_cb_wrapper, this);
    send_pair_request();
}

void LoraNodeProtocol::loop() {
    unsigned long now = millis();

    if (!m_paired) {
        if (now - m_last_pair_ms >= m_pair_interval_ms &&
            m_pair_attempts < s_max_pair_attempts) {
            m_last_pair_ms = now;
            m_pair_attempts++;
            send_pair_request();
        }
    } else {
        if (now - m_last_heartbeat_ms >= m_heartbeat_interval_ms) {
            m_last_heartbeat_ms = now;
            send_heartbeat();
        }
        if (now - m_last_state_ms >= m_state_interval_ms) {
            m_last_state_ms = now;
            send_sensor_data();
        }
    }
}

void LoraNodeProtocol::force_repair() {
    m_paired = false;
    m_pair_attempts = 0;
    m_last_pair_ms = 0;
}

void LoraNodeProtocol::send_pair_request() {
    lora_pair_request_t req;
    memset(&req, 0, sizeof(req));
    req.msg_type = LORA_MSG_PAIR_REQUEST;
    req.sequence = m_sequence++;
    memcpy(req.sensor_id, m_mac, 6);
    req.payload_len = 1;
    req.sensor_type = callbacks.get_sensor_type ? callbacks.get_sensor_type() : 0;
    strncpy(req.device_name, m_device_name, sizeof(req.device_name) - 1);
    req.device_name[sizeof(req.device_name) - 1] = '\0';
    m_radio->send((const uint8_t*)&req, sizeof(req));
}

void LoraNodeProtocol::send_sensor_data() {
    uint8_t payload[LORA_MAX_PAYLOAD];
    uint8_t payload_len = 0;
    if (callbacks.get_sensor_payload) {
        payload_len = callbacks.get_sensor_payload(payload, LORA_MAX_PAYLOAD);
    }
    uint8_t buf[LORA_HEADER_SIZE + payload_len];
    lora_frame_t* frame = (lora_frame_t*)buf;
    frame->msg_type = LORA_MSG_SENSOR_DATA;
    frame->sequence = m_sequence++;
    memcpy(frame->sensor_id, m_mac, 6);
    frame->rssi = 0;
    frame->payload_len = payload_len;
    if (payload_len > 0) memcpy(frame->payload, payload, payload_len);
    m_radio->send(buf, LORA_HEADER_SIZE + payload_len);
}

void LoraNodeProtocol::send_heartbeat() {
    uint8_t buf[LORA_HEADER_SIZE];
    lora_frame_t* frame = (lora_frame_t*)buf;
    frame->msg_type = LORA_MSG_HEARTBEAT;
    frame->sequence = m_sequence++;
    memcpy(frame->sensor_id, m_mac, 6);
    frame->rssi = 0;
    frame->payload_len = 0;
    m_radio->send(buf, LORA_HEADER_SIZE);
}

void LoraNodeProtocol::handle_frame(const uint8_t* data, size_t len, int16_t rssi, void* arg) {
    (void)rssi;
    (void)arg;
    if (len < LORA_HEADER_SIZE) return;
    const lora_frame_t* frame = (const lora_frame_t*)data;

    if (frame->msg_type == LORA_MSG_PAIR_RESPONSE) {
        if (len >= sizeof(lora_pair_response_t)) {
            const lora_pair_response_t* resp = (const lora_pair_response_t*)data;
            if (memcmp(resp->sensor_id, m_mac, 6) == 0) {
                m_paired = true;
                m_slot = resp->assigned_slot;
                m_pair_attempts = 0;
                if (callbacks.on_paired) callbacks.on_paired(m_slot);
                send_sensor_data();
                m_last_state_ms = millis();
            }
        }
    } else if (frame->msg_type == LORA_MSG_COMMAND) {
        if (len >= sizeof(lora_command_t)) {
            const lora_command_t* cmd = (const lora_command_t*)data;
            if (memcmp(cmd->sensor_id, m_mac, 6) == 0) {
                if (cmd->command == 0xFF) {
                    if (callbacks.on_restart) callbacks.on_restart();
                } else {
                    if (callbacks.on_command) callbacks.on_command(cmd->command);
                }
            }
        }
    }
}

void LoraNodeProtocol::rx_cb_wrapper(const uint8_t* data, size_t len, int16_t rssi, void* arg) {
    LoraNodeProtocol* self = (LoraNodeProtocol*)arg;
    self->handle_frame(data, len, rssi, arg);
}
