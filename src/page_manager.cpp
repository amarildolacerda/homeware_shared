#include "page_manager.h"
#include <Arduino.h>

PageManager::PageManager(DisplayInterface* display)
    : m_display(display)
    , m_last_switch_ms(0)
    , m_current(0)
    , m_count(0)
{
    for (int i = 0; i < MAX_PAGES; i++) m_pages[i] = nullptr;
}

void PageManager::set_page_interval(unsigned long ms) {
    if (ms > 0) m_interval_ms = ms;
}

bool PageManager::add_page(page_render_t render_fn) {
    if (m_count >= MAX_PAGES || !render_fn) return false;
    m_pages[m_count++] = render_fn;
    return true;
}

void PageManager::set_footer(page_render_t footer_fn) {
    m_footer = footer_fn;
}

int PageManager::loop() {
    if (!m_display || m_count == 0) return -1;

    unsigned long now = millis();
    if (now - m_last_switch_ms < m_interval_ms) return m_current;
    m_last_switch_ms = now;

    m_current = (m_current + 1) % m_count;

    m_display->clear();
    if (m_pages[m_current]) {
        (*m_pages[m_current])(*m_display);
    }
    if (m_footer) {
        (*m_footer)(*m_display);
    }
    m_display->display();
    return m_current;
}
