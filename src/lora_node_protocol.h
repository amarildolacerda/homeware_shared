#ifndef HW_SHARED_LORA_NODE_PROTOCOL_H
#define HW_SHARED_LORA_NODE_PROTOCOL_H

#include "node_protocol.h"
#include "radio_interface.h"
#include <stdint.h>

class LoraNodeProtocol : public NodeProtocol {
public:
    LoraNodeProtocol(RadioInterface* radio);

    void begin() override;
    void loop() override;
    bool is_paired() const override { return m_paired; }
    uint8_t assigned_slot() const override { return m_slot; }
    void force_repair() override;

    void publish_state();
    int16_t last_rssi() const { return m_last_rssi; }
    uint32_t rx_count() const { return m_rx_count; }
    uint32_t tx_count() const { return m_tx_count; }

    void set_mac(const uint8_t* mac);
    void set_pair_interval(unsigned long ms) { m_pair_interval_ms = ms; }
    void set_heartbeat_interval(unsigned long ms) { m_heartbeat_interval_ms = ms; }
    void set_state_interval(unsigned long ms) { m_state_interval_ms = ms; }
    void set_device_name(const char* name);

private:
    RadioInterface* m_radio;
    uint8_t m_mac[6];
    bool m_paired;
    uint8_t m_slot;
    char m_device_name[32];
    uint16_t m_sequence;
    unsigned long m_pair_interval_ms;
    unsigned long m_heartbeat_interval_ms;
    unsigned long m_state_interval_ms;
    unsigned long m_last_pair_ms;
    unsigned long m_last_heartbeat_ms;
    unsigned long m_last_state_ms;
    uint8_t m_pair_attempts;
    int16_t m_last_rssi;
    uint32_t m_rx_count;
    uint32_t m_tx_count;

    void send_pair_request();
    void send_sensor_data();
    void send_heartbeat();
    void handle_frame(const uint8_t* data, size_t len, int16_t rssi, void* arg);
    static void rx_cb_wrapper(const uint8_t* data, size_t len, int16_t rssi, void* arg);
};

#endif
