#ifdef DISPLAY_TTGO

#include "display_ttgo.h"
#include <stdarg.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

Ssd1306DisplayTtgo::Ssd1306DisplayTtgo(const Ssd1306TtgoConfig& cfg)
    : m_cfg(cfg)
#ifdef DISPLAY_TTGO
    , m_display(cfg.width, cfg.height, &Wire, cfg.rst)
#endif
{}

bool Ssd1306DisplayTtgo::begin() {
#ifdef DISPLAY_TTGO
    Wire.begin(m_cfg.sda, m_cfg.scl);
    return m_display.begin(SSD1306_SWITCHCAPVCC, m_cfg.addr);
#else
    return false;
#endif
}

void Ssd1306DisplayTtgo::clear() {
#ifdef DISPLAY_TTGO
    m_display.clearDisplay();
#endif
}

void Ssd1306DisplayTtgo::set_cursor(int x, int y) {
#ifdef DISPLAY_TTGO
    m_display.setCursor(x, y);
#endif
}

void Ssd1306DisplayTtgo::set_text_size(int size) {
#ifdef DISPLAY_TTGO
    m_display.setTextSize(size);
#endif
}

void Ssd1306DisplayTtgo::print(const char* str) {
#ifdef DISPLAY_TTGO
    m_display.print(str);
#endif
}

void Ssd1306DisplayTtgo::printf(const char* fmt, ...) {
#ifdef DISPLAY_TTGO
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    m_display.print(buf);
#endif
}

void Ssd1306DisplayTtgo::display() {
#ifdef DISPLAY_TTGO
    m_display.display();
#endif
}

int Ssd1306DisplayTtgo::width() const { return m_cfg.width; }
int Ssd1306DisplayTtgo::height() const { return m_cfg.height; }

#endif // DISPLAY_TTGO
