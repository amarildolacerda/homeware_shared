// shared/src/node_protocol.h
#ifndef HW_SHARED_NODE_PROTOCOL_H
#define HW_SHARED_NODE_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

struct NodeCallbacks {
    uint8_t (*get_sensor_type)();
    uint8_t (*get_sensor_payload)(uint8_t* buf, uint8_t max_len);
    void    (*on_command)(uint8_t command);
    void    (*on_paired)(uint8_t slot);
    void    (*on_restart)();
    void    (*on_forward)(const uint8_t* data, size_t len, const uint8_t* mac);
    void    (*on_pairing_failed)();  /* called when all pair attempts exhausted — try next AP */
};

class NodeProtocol {
public:
    virtual ~NodeProtocol() {}

    // Core lifecycle
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool is_paired() const = 0;
    virtual uint8_t assigned_slot() const = 0;
    virtual void force_repair() = 0;

    // State publishing
    virtual void publish_state() {}

    // Gateway management
    virtual const uint8_t* gateway_mac() const { return nullptr; }
    virtual void set_gateway_mac(const uint8_t* mac) { (void)mac; }
    virtual void save_gateway_mac() {}
    virtual void load_gateway_mac() {}

    // Configuration
    virtual void set_mac(const uint8_t* mac) { (void)mac; }
    virtual void set_device_name(const char* name) { (void)name; }
    virtual void set_device_id(const char* id) { (void)id; }
    virtual void set_pair_interval(unsigned long ms) { (void)ms; }
    virtual void set_heartbeat_interval(unsigned long ms) { (void)ms; }
    virtual void set_state_interval(unsigned long ms) { (void)ms; }

    // Counters
    virtual uint32_t tx_count() const { return 0; }
    virtual uint32_t rx_count() const { return 0; }

    NodeCallbacks callbacks;
};

#endif
