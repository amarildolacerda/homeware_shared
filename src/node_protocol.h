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
};

class NodeProtocol {
public:
    virtual ~NodeProtocol() {}
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool is_paired() const = 0;
    virtual uint8_t assigned_slot() const = 0;
    virtual void force_repair() = 0;
    NodeCallbacks callbacks;
};

#endif
