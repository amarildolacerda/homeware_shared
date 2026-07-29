// shared/src/lora_spi_radio.cpp
#ifdef LORA_DEVICE

#include "lora_spi_radio.h"
#include "lora_protocol.h"
#include <LoRa.h>
#include <SPI.h>

LoraSpiRadio::LoraSpiRadio(const LoraSpiConfig& cfg) : m_cfg(cfg) {}

int LoraSpiRadio::init() {
    SPI.begin(m_cfg.sck, m_cfg.miso, m_cfg.mosi, m_cfg.ss);
    LoRa.setPins(m_cfg.ss, m_cfg.rst, m_cfg.dio0);
    if (!LoRa.begin(m_cfg.freq * 1E6)) return -1;
    LoRa.setSpreadingFactor(m_cfg.sf);
    LoRa.setSignalBandwidth(m_cfg.bw);
    LoRa.setCodingRate4(m_cfg.cr);
    LoRa.setTxPower(m_cfg.tx_power);
    LoRa.setPreambleLength(m_cfg.preamble);
    LoRa.receive();
    m_ok = true;
    return 0;
}

int LoraSpiRadio::send(const uint8_t* data, size_t len) {
    if (!m_ok) return -1;
    LoRa.beginPacket();
    LoRa.write(data, len);
    int ret = LoRa.endPacket() ? 0 : -1;
    LoRa.receive();
    return ret;
}

void LoraSpiRadio::loop() {
    if (!m_ok) return;
    handle_rx();
}

bool LoraSpiRadio::is_ready() const {
    return m_ok;
}

void LoraSpiRadio::handle_rx() {
    int len = LoRa.parsePacket();
    if (len <= 0 || len > (int)sizeof(m_rx_buf)) return;
    int i = 0;
    while (LoRa.available() && i < (int)sizeof(m_rx_buf)) {
        m_rx_buf[i++] = LoRa.read();
    }
    m_rx_len = i;
    int16_t rssi = LoRa.packetRssi();
    if (m_rx_len >= LORA_HEADER_SIZE && m_rx_cb) {
        m_rx_cb(m_rx_buf, m_rx_len, rssi, m_rx_arg);
    }
}

#endif // LORA_DEVICE
