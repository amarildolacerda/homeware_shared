#ifndef HW_SHARED_DISPLAY_TTGO_H
#define HW_SHARED_DISPLAY_TTGO_H

#include "display_interface.h"
#include <stdint.h>
#ifdef DISPLAY_TTGO
#include <Adafruit_SSD1306.h>
#endif

struct Ssd1306TtgoConfig {
    int8_t sda = 21;
    int8_t scl = 22;
    int8_t rst = -1;
    uint8_t addr = 0x3C;
    int width = 128;
    int height = 64;
};

class Ssd1306DisplayTtgo : public DisplayInterface {
public:
    Ssd1306DisplayTtgo(const Ssd1306TtgoConfig& cfg = Ssd1306TtgoConfig{});
    bool begin() override;
    void clear() override;
    void set_cursor(int x, int y) override;
    void set_text_size(int size) override;
    void print(const char* str) override;
    void printf(const char* fmt, ...) override;
    void display() override;
    int width() const override;
    int height() const override;
private:
    Ssd1306TtgoConfig m_cfg;
#ifdef DISPLAY_TTGO
    Adafruit_SSD1306 m_display;
#endif
};

#endif
