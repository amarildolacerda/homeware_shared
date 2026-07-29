// shared/src/lora_spi_radio.h
#ifndef HW_SHARED_LORA_SPI_RADIO_H
#define HW_SHARED_LORA_SPI_RADIO_H

#include "radio_interface.h"
#include <stdint.h>
#include <stddef.h>

struct LoraSpiConfig {
    int8_t ss    = 18;
    int8_t rst   = 14;
    int8_t dio0  = -1;
    int8_t sck   = 5;
    int8_t miso  = 19;
    int8_t mosi  = 27;
    float  freq  = 868.0;
    uint8_t sf   = 10;
    float  bw    = 125E3;
    uint8_t cr   = 7;
    uint8_t preamble = 8;
    int8_t  tx_power = 17;
};

class LoraSpiRadio : public RadioInterface {
public:
    LoraSpiRadio(const LoraSpiConfig& cfg);
    int init() override;
    int send(const uint8_t* data, size_t len) override;
    void loop() override;
    bool is_ready() const override;
private:
    LoraSpiConfig m_cfg;
    bool m_ok = false;
    uint8_t m_rx_buf[256];
    int m_rx_len = 0;
    void handle_rx();
};

#endif
