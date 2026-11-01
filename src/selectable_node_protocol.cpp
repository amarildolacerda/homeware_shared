// shared/src/selectable_node_protocol.cpp
#include "selectable_node_protocol.h"

#ifdef RADIO_SELECT_ENABLED

SelectableNodeProtocol::SelectableNodeProtocol()
    : m_mode(RADIO_MODE_DEFAULT)
{
}

NodeProtocol& SelectableNodeProtocol::active()
{
    if (m_mode == RADIO_MODE_TCP) return m_tcp;
    return m_espnow;
}

const NodeProtocol& SelectableNodeProtocol::active() const
{
    if (m_mode == RADIO_MODE_TCP) return m_tcp;
    return m_espnow;
}

void SelectableNodeProtocol::set_mode(uint8_t mode)
{
    if (!radio_mode_compiled(mode)) return;
    m_mode = mode;
    radio_mode_set_active(mode);
}

void SelectableNodeProtocol::begin()
{
    active().begin();
}

void SelectableNodeProtocol::loop()
{
    active().loop();
}

bool SelectableNodeProtocol::is_paired() const
{
    return active().is_paired();
}

uint8_t SelectableNodeProtocol::assigned_slot() const
{
    return active().assigned_slot();
}

void SelectableNodeProtocol::force_repair()
{
    active().force_repair();
}

void SelectableNodeProtocol::publish_state()
{
    active().publish_state();
}

const uint8_t* SelectableNodeProtocol::gateway_mac() const
{
    return active().gateway_mac();
}

/* Configuração é replicada nos dois protocolos: só um vai rodar, mas os
   setters são chamados antes de begin() (quando o modo já está definido) e
   não há custo em manter o inativo sincronizado. */
void SelectableNodeProtocol::set_gateway_mac(const uint8_t* mac)
{
    m_espnow.set_gateway_mac(mac);
    m_tcp.set_gateway_mac(mac);
}

void SelectableNodeProtocol::save_gateway_mac()
{
    m_espnow.save_gateway_mac();
    m_tcp.save_gateway_mac();
}

void SelectableNodeProtocol::load_gateway_mac()
{
    m_espnow.load_gateway_mac();
    m_tcp.load_gateway_mac();
}

void SelectableNodeProtocol::set_mac(const uint8_t* mac)
{
    m_espnow.set_mac(mac);
    m_tcp.set_mac(mac);
}

void SelectableNodeProtocol::set_device_name(const char* name)
{
    m_espnow.set_device_name(name);
    m_tcp.set_device_name(name);
}

void SelectableNodeProtocol::set_pair_interval(unsigned long ms)
{
    m_espnow.set_pair_interval(ms);
    m_tcp.set_pair_interval(ms);
}

void SelectableNodeProtocol::set_heartbeat_interval(unsigned long ms)
{
    m_espnow.set_heartbeat_interval(ms);
    m_tcp.set_heartbeat_interval(ms);
}

void SelectableNodeProtocol::set_state_interval(unsigned long ms)
{
    m_espnow.set_state_interval(ms);
    m_tcp.set_state_interval(ms);
}

uint32_t SelectableNodeProtocol::tx_count() const
{
    return active().tx_count();
}

uint32_t SelectableNodeProtocol::rx_count() const
{
    return active().rx_count();
}

bool SelectableNodeProtocol::has_gateway() const
{
    return active().has_gateway();
}

// ── Extras TCP ──

void SelectableNodeProtocol::set_device_id(const char* id)
{
    m_tcp.set_device_id(id);
}

void SelectableNodeProtocol::set_hub_ip(const char* ip)
{
    // Persiste mesmo em modo ESP-NOW: a config fica pronta para quando o
    // protocolo for trocado para TCP.
    m_tcp.set_hub_ip(ip);
}

const char* SelectableNodeProtocol::hub_ip_str() const
{
    return m_tcp.hub_ip_str();
}

bool SelectableNodeProtocol::hub_ip_configured() const
{
    return m_tcp.hub_ip_configured();
}

IPAddress SelectableNodeProtocol::gateway_ip() const
{
    return m_tcp.gateway_ip();
}

bool SelectableNodeProtocol::send_to_hub(const char* endpoint, const String& payload)
{
    if (!uses_tcp()) return false;
    return m_tcp.send_to_hub(endpoint, payload);
}

bool SelectableNodeProtocol::send_to_hub_get(const char* endpoint, String& response)
{
    if (!uses_tcp()) return false;
    return m_tcp.send_to_hub_get(endpoint, response);
}

bool SelectableNodeProtocol::fetch_hub_devices(char* json_out, size_t max_len)
{
    if (!uses_tcp()) return false;
    return m_tcp.fetch_hub_devices(json_out, max_len);
}

#endif // RADIO_SELECT_ENABLED
