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

    // Optional operations with safe defaults
    virtual bool send_command(const uint8_t* mac, uint8_t state)
        { (void)mac; (void)state; return false; }
    virtual bool send_restart(const uint8_t* mac)
        { (void)mac; return false; }
    virtual unsigned long get_rx_count() const { return 0; }
    virtual unsigned long get_ack_count() const { return 0; }
    virtual unsigned long get_crc_errors() const { return 0; }
    virtual bool start_pairing() { return false; }
    virtual void stop_pairing() {}
    virtual bool is_pairing() const { return false; }
    virtual unsigned long pairing_remaining_ms() const { return 0; }
    virtual uint8_t* get_radio_mac() { return nullptr; }
    virtual void announce() {}
    virtual void broadcast_time_sync(uint32_t epoch) { (void)epoch; }

protected:
    rx_callback_t m_rx_cb = nullptr;
    void* m_rx_arg = nullptr;
};

#endif
