// shared/src/selectable_node_protocol.h
// NodeProtocol que escolhe em runtime entre ESP-NOW e TCP/HTTP.
//
// Requer RADIO_SELECT_ENABLED + ESPNOW_ENABLED + TCP_ENABLED (os dois
// protocolos no mesmo firmware). O modo é lido da EEPROM (radio_mode.h) e
// aplicado em set_mode() ANTES de begin(). Trocar o modo em runtime exige
// restart do device (o ESP-NOW registra callbacks globais no begin()).
#ifndef HW_SHARED_SELECTABLE_NODE_PROTOCOL_H
#define HW_SHARED_SELECTABLE_NODE_PROTOCOL_H

#if defined(RADIO_SELECT_ENABLED) && (!defined(ESPNOW_ENABLED) || !defined(TCP_ENABLED))
#error "SelectableNodeProtocol requires ESPNOW_ENABLED and TCP_ENABLED"
#endif

#include "node_protocol.h"
#include "espnow_node_protocol.h"
#include "tcp_node_protocol.h"
#include "radio_mode.h"
#include <Arduino.h>

class SelectableNodeProtocol : public NodeProtocol {
public:
    SelectableNodeProtocol();

    // Define o protocolo ativo. Chamar antes de begin().
    void set_mode(uint8_t mode);
    uint8_t mode() const { return m_mode; }
    bool uses_tcp() const override { return m_mode == RADIO_MODE_TCP; }

    void begin() override;
    void loop() override;
    bool is_paired() const override;
    uint8_t assigned_slot() const override;
    void force_repair() override;
    void publish_state() override;

    const uint8_t* gateway_mac() const override;
    void set_gateway_mac(const uint8_t* mac) override;
    void save_gateway_mac() override;
    void load_gateway_mac() override;

    void set_mac(const uint8_t* mac) override;
    void set_device_name(const char* name) override;
    void set_pair_interval(unsigned long ms) override;
    void set_heartbeat_interval(unsigned long ms) override;
    void set_state_interval(unsigned long ms) override;

    uint32_t tx_count() const override;
    uint32_t rx_count() const override;
    bool has_gateway() const override;

    // Extras TCP (no-op/false em modo ESP-NOW)
    void set_device_id(const char* id);
    void set_hub_ip(const char* ip);
    const char* hub_ip_str() const;
    bool hub_ip_configured() const;
    IPAddress gateway_ip() const;
    bool send_to_hub(const char* endpoint, const String& payload);
    bool send_to_hub_get(const char* endpoint, String& response);
    bool fetch_hub_devices(char* json_out, size_t max_len);

private:
    uint8_t m_mode;

    // Ordem de declaração: os dois protocolos coexistem no binário, mas só o
    // ativo recebe begin()/loop()/publicação.
    EspnowNodeProtocol m_espnow;
    TcpNodeProtocol m_tcp;

    // Copia NodeProtocol::callbacks para o protocolo ativo (ver regra 56).
    void sync_callbacks();

    NodeProtocol& active();
    const NodeProtocol& active() const;
};

#endif
