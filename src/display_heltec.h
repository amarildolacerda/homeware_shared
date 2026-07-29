#ifndef HW_SHARED_DISPLAY_HELTEC_H
#define HW_SHARED_DISPLAY_HELTEC_H

#include "display_interface.h"
#include <stdint.h>
#ifdef DISPLAY_HELTEC
#include <HT_SSD1306Wire.h>
#endif

struct Ssd1306HeltecConfig {
    int8_t sda = 4;
    int8_t scl = 15;
    int8_t rst = 16;
    uint8_t addr = 0x3C;
    int width = 128;
    int height = 64;
};

class Ssd1306DisplayHeltec : public DisplayInterface {
public:
    Ssd1306DisplayHeltec(const Ssd1306HeltecConfig& cfg = Ssd1306HeltecConfig{});
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
    Ssd1306HeltecConfig m_cfg;
    int m_cx = 0, m_cy = 0;
#ifdef DISPLAY_HELTEC
    SSD1306Wire m_display;
#endif
};

#endif
