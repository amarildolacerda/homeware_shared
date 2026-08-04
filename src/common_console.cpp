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
    // Nao fechar automaticamente em !connected(): no ESP32 o connected()
    // pode oscilar apos um write e derrubar a sessao sem motivo. O write()
    // ja descarta bytes se o buffer estiver cheio; a conexao e encerrada
    // so quando o client realmente se desconecta (detectado no read/loop).
}

int ConsoleOutput::telnet_available() {
    if (m_client && m_client.connected()) return m_client.available();
    return 0;
}

int ConsoleOutput::telnet_read() {
    if (m_client && m_client.connected()) return m_client.read();
    return -1;
}

bool ConsoleOutput::telnet_connected() {
    return m_client && m_client.connected();
}

size_t ConsoleOutput::write(uint8_t c) {
    Serial.write(c);
    if (m_client && m_client.connected()) {
        // Envio direto; o WiFiClient do ESP32 lida com o buffering do TCP.
        // Em caso de cliente lento, o write pode falhar silenciosamente
        // (bytes perdidos) mas a conexao permanece aberta.
        m_client.write(c);
    }
    return 1;
}

size_t ConsoleOutput::write(const uint8_t *buffer, size_t size) {
    Serial.write(buffer, size);
    if (m_client && m_client.connected()) {
        m_client.write((const char*)buffer, size);
    }
    return size;
}

ConsoleOutput console;
