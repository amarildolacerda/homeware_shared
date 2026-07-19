#include "common_console.h"

void ConsoleOutput::begin() {
    m_server.begin();
    m_banner[0] = 0;
}

void ConsoleOutput::loop() {
    if (m_server.hasClient()) {
        if (m_client && m_client.connected()) {
            m_client.stop();
        }
        m_client = m_server.available();
        if (m_client) {
            m_client.setNoDelay(true);
            if (m_banner[0]) m_client.printf("\r\n=== %s ===\r\n", m_banner);
            m_client.print("Console remoto conectado.\r\n");
        }
    }
    if (m_client && !m_client.connected()) {
        m_client.stop();
    }
}

int ConsoleOutput::telnet_available() {
    if (m_client && m_client.connected()) return m_client.available();
    return 0;
}

int ConsoleOutput::telnet_read() {
    if (m_client && m_client.connected()) return m_client.read();
    return -1;
}

size_t ConsoleOutput::write(uint8_t c) {
    Serial.write(c);
    if (m_client && m_client.connected()) {
        // Nao bloquear: se o buffer TCP do telnet estiver cheio (client nao
        // consome), pular o byte em vez de travar o core (WDT reset).
        if (m_client.availableForWrite() > 0)
            m_client.write(c);
        else
            m_client.stop();
    }
    return 1;
}

size_t ConsoleOutput::write(const uint8_t *buffer, size_t size) {
    Serial.write(buffer, size);
    if (m_client && m_client.connected()) {
        int space = m_client.availableForWrite();
        if (space > 0) {
            if ((size_t)space > size) space = (int)size;
            m_client.write((const char*)buffer, space);
        } else {
            m_client.stop();
        }
    }
    return size;
}

ConsoleOutput console;
