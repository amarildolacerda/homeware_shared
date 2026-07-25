#ifndef HW_SHARED_RADIO_INTERFACE_H
#define HW_SHARED_RADIO_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

class RadioInterface {
public:
    virtual ~RadioInterface() {}

    virtual int init() = 0;
    virtual int send(const uint8_t* data, size_t len) = 0;
    virtual void loop() = 0;
    virtual bool is_ready() const = 0;

    using rx_callback_t = void (*)(const uint8_t* data, size_t len,
                                    int16_t rssi, void* arg);

    void set_rx_callback(rx_callback_t cb, void* arg = nullptr) {
        m_rx_cb = cb;
        m_rx_arg = arg;
    }

protected:
    rx_callback_t m_rx_cb = nullptr;
    void* m_rx_arg = nullptr;
};

#endif
