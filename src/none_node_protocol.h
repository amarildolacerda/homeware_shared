// shared/src/none_node_protocol.h
// Stub radio for standalone mode (no hub, no radio) — lamp works with Alexa + button + dashboard only.
#ifndef HW_SHARED_NONE_NODE_PROTOCOL_H
#define HW_SHARED_NONE_NODE_PROTOCOL_H

#include "node_protocol.h"

class NoneRadio : public NodeProtocol
{
public:
    void begin() override {}
    void loop() override {}
    bool is_paired() const override { return false; }
    uint8_t assigned_slot() const override { return 0xFF; }
    bool has_gateway() const override { return false; }

    void force_repair() override {}
    const uint8_t *gateway_mac() const override { return m_gateway_mac; }
    void set_gateway_mac(const uint8_t *mac) override
    {
        if (mac)
            memcpy(m_gateway_mac, mac, 6);
    }

private:
    uint8_t m_gateway_mac[6] = {0};
};

#endif
