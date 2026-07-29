#ifdef DISPLAY_HELTEC

#include "display_heltec.h"
#include <HT_SSD1306Wire.h>
#include <Wire.h>
#include <stdio.h>
#include <stdarg.h>

Ssd1306DisplayHeltec::Ssd1306DisplayHeltec(const Ssd1306HeltecConfig& cfg)
    : m_cfg(cfg)
#ifdef DISPLAY_HELTEC
    , m_display(cfg.addr, 400000, cfg.sda, cfg.scl, GEOMETRY_128_64, cfg.rst)
#endif
{}

bool Ssd1306DisplayHeltec::begin() {
#ifdef DISPLAY_HELTEC
    pinMode(m_cfg.rst, OUTPUT);
    digitalWrite(m_cfg.rst, LOW);
    delay(50);
    digitalWrite(m_cfg.rst, HIGH);
    delay(50);
    if (!m_display.init()) return false;
    m_display.displayOn();
    m_display.flipScreenVertically();
    return true;
#else
    return false;
#endif
}

void Ssd1306DisplayHeltec::clear() {
#ifdef DISPLAY_HELTEC
    m_display.clear();
#endif
}

void Ssd1306DisplayHeltec::set_cursor(int x, int y) {
    m_cx = x; m_cy = y;
}

void Ssd1306DisplayHeltec::set_text_size(int size) {
#ifdef DISPLAY_HELTEC
    switch (size) {
        case 1: m_display.setFont(ArialMT_Plain_10); break;
        case 2: m_display.setFont(ArialMT_Plain_16); break;
        default: m_display.setFont(ArialMT_Plain_10); break;
    }
#endif
}

void Ssd1306DisplayHeltec::print(const char* str) {
#ifdef DISPLAY_HELTEC
    m_display.drawString(m_cx, m_cy, str);
    m_cx += m_display.getStringWidth(str);
#endif
}

void Ssd1306DisplayHeltec::printf(const char* fmt, ...) {
#ifdef DISPLAY_HELTEC
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    print(buf);
#endif
}

void Ssd1306DisplayHeltec::display() {
#ifdef DISPLAY_HELTEC
    m_display.display();
#endif
}

int Ssd1306DisplayHeltec::width() const { return m_cfg.width; }
int Ssd1306DisplayHeltec::height() const { return m_cfg.height; }

#endif // DISPLAY_HELTEC
