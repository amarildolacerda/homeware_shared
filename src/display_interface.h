#ifndef HW_SHARED_DISPLAY_INTERFACE_H
#define HW_SHARED_DISPLAY_INTERFACE_H

#include <stdint.h>

class DisplayInterface {
public:
    virtual ~DisplayInterface() {}
    virtual bool begin() = 0;
    virtual void clear() = 0;
    virtual void set_cursor(int x, int y) = 0;
    virtual void set_text_size(int size) = 0;
    virtual void print(const char* str) = 0;
    virtual void printf(const char* fmt, ...) = 0;
    virtual void display() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

#endif
