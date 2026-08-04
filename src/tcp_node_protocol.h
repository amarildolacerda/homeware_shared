// shared/src/tcp_node_protocol.h
#ifndef HW_SHARED_TCP_NODE_PROTOCOL_H
#define HW_SHARED_TCP_NODE_PROTOCOL_H

#include "node_protocol.h"
#include "tcp_protocol.h"
#include "sensor_type.h"
#include <stdint.h>
#include <WiFiUdp.h>

class TcpNodeProtocol : public NodeProtocol {
public:
    TcpNodeProtocol();

    void begin() override;
    void loop() override;
    bool is_paired() const override { return m_registered; }
    uint8_t assigned_slot() const override { return m_slot; }
    void force_repair() override;

    void publish_state() override;

    const uint8_t* gateway_mac() const override { return m_gateway_mac; }
    void set_gateway_mac(const uint8_t* mac) override;
    void save_gateway_mac() override;
    void load_gateway_mac() override;

    void set_mac(const uint8_t* mac) override;
    void set_device_name(const char* name) override;
    void set_pair_interval(unsigned long ms) override { m_pair_interval_ms = ms; }
    void set_heartbeat_interval(unsigned long ms) override { m_heartbeat_interval_ms = ms; }
    void set_state_interval(unsigned long ms) override { m_state_interval_ms = ms; }

    uint32_t tx_count() const override { return m_tx_count; }
    uint32_t rx_count() const override { return m_rx_count; }

    // TCP-specific: set device_id (needed before begin)
    void set_device_id(const char* id);

private:
    uint8_t m_mac[6];
    uint8_t m_gateway_mac[6];
    char m_device_name[32];
    char m_device_id[32];
    uint8_t m_slot;
    bool m_registered;
    bool m_hub_found;
    int m_retry_count;

    char m_hub_ip[16];
    uint16_t m_hub_port;

    unsigned long m_pair_interval_ms;
    unsigned long m_heartbeat_interval_ms;
    unsigned long m_state_interval_ms;
    unsigned long m_last_state_ms;
    unsigned long m_last_heartbeat_ms;
    unsigned long m_last_discover_ms;
    unsigned long m_last_register_ms;
    unsigned long m_last_command_check_ms;
    unsigned long m_tx_count;
    unsigned long m_rx_count;

    WiFiUDP m_udp;

    void send_udp_discover();
    void handle_udp_announce();
    bool register_with_hub();
    bool send_to_hub(const char* endpoint, const String& payload);
    void check_commands();
};

#endif
