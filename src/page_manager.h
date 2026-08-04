#ifndef HW_SHARED_PAGE_MANAGER_H
#define HW_SHARED_PAGE_MANAGER_H

#include "display_interface.h"
#include <stdint.h>

using page_render_t = void (*)(DisplayInterface&);

class PageManager {
public:
    PageManager(DisplayInterface* display);

    void set_page_interval(unsigned long ms);
    bool add_page(page_render_t render_fn);
    void set_footer(page_render_t footer_fn);
    int  current() const { return m_current; }
    int  loop();

private:
    DisplayInterface* m_display;
    unsigned long m_interval_ms = 5000;
    unsigned long m_last_switch_ms = 0;
    uint8_t m_current = 0;
    uint8_t m_count = 0;
    static const uint8_t MAX_PAGES = 4;
    page_render_t m_pages[MAX_PAGES];
    page_render_t m_footer = nullptr;
};

#endif
