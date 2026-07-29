#ifndef HW_SHARED_ESPNOW_NODE_PROTOCOL_H
#define HW_SHARED_ESPNOW_NODE_PROTOCOL_H

#include "node_protocol.h"
#include <stdint.h>
#include <string.h>

class EspnowNodeProtocol : public NodeProtocol {
public:
    EspnowNodeProtocol();

    void begin() override;
    void loop() override;
    bool is_paired() const override { return m_paired; }
    uint8_t assigned_slot() const override { return m_slot; }
    void force_repair() override;

    void publish_state();
    void on_send_done(const uint8_t* mac, uint8_t status);

    int16_t last_rssi() const { return m_last_rssi; }
    uint32_t tx_count() const { return m_tx_count; }
    uint32_t rx_count() const { return m_rx_count; }
    uint8_t* my_mac() { return m_mac; }

    void set_mac(const uint8_t* mac);
    void set_device_name(const char* name);
    void set_gateway_mac(const uint8_t* mac) { memcpy(m_gateway_mac, mac, 6); }
    const uint8_t* gateway_mac() const { return m_gateway_mac; }
    void load_gateway_mac();
    void save_gateway_mac();
    void set_pair_interval(unsigned long ms) { m_pair_interval_ms = ms; }
    void set_heartbeat_interval(unsigned long ms) { m_heartbeat_interval_ms = ms; }
    void set_state_interval(unsigned long ms) { m_state_interval_ms = ms; }

    void handle_frame(const uint8_t* mac, const uint8_t* data, size_t len);

private:
    uint8_t m_mac[6];
    uint8_t m_gateway_mac[6];
    bool m_paired;
    uint8_t m_slot;
    uint16_t m_sequence;
    char m_device_name[32];
    bool m_espnow_ready;
    bool m_ack_received;
    int m_retries_left;
    unsigned long m_pair_interval_ms;
    unsigned long m_heartbeat_interval_ms;
    unsigned long m_state_interval_ms;
    unsigned long m_last_pair_ms;
    unsigned long m_last_heartbeat_ms;
    unsigned long m_last_state_ms;
    unsigned long m_send_deadline;
    unsigned long m_retry_delay_ms;
    int m_pair_attempts;
    uint8_t m_pair_attempts_max;
    int16_t m_last_rssi;
    uint32_t m_tx_count;
    uint32_t m_rx_count;
    uint16_t m_last_send_sequence;
    uint8_t m_fw_version[8];

    enum SendState { SEND_IDLE, SEND_WAIT_ACK, SEND_RETRY_DELAY, SEND_RETRY_WAIT_ACK };
    SendState m_send_state;

    void send_pair_request();
    void send_sensor_data();
    void send_heartbeat();
};

#endif
