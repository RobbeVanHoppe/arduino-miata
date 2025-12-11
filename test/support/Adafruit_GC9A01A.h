#pragma once

#include <stdint.h>

class Adafruit_GC9A01A {
public:
    explicit Adafruit_GC9A01A(uint8_t csPin = 0, uint8_t dcPin = 0, uint8_t rstPin = 0)
            : width_(240), height_(240) {
        (void) csPin;
        (void) dcPin;
        (void) rstPin;
    }

    int16_t width() const { return width_; }
    int16_t height() const { return height_; }

    void setTextSize(uint8_t size) { (void) size; }
    void setTextWrap(bool wrap) { (void) wrap; }
    void setTextColor(uint16_t color, uint16_t background) {
        (void) color;
        (void) background;
    }
    void setCursor(int16_t x, int16_t y) {
        (void) x;
        (void) y;
    }

    void fillScreen(uint16_t color) { (void) color; }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        (void) x;
        (void) y;
        (void) w;
        (void) h;
        (void) color;
    }

    void getTextBounds(const char *text,
                       int16_t x,
                       int16_t y,
                       int16_t *x1,
                       int16_t *y1,
                       uint16_t *w,
                       uint16_t *h) {
        (void) text;
        (void) x;
        (void) y;
        if (x1) *x1 = 0;
        if (y1) *y1 = 0;
        if (w) *w = 0;
        if (h) *h = 0;
    }

    void print(const char *text) { (void) text; }
    void print(const class String &text) { (void) text; }

    void setDimensions(int16_t width, int16_t height) {
        width_ = width;
        height_ = height;
    }

private:
    int16_t width_;
    int16_t height_;
};

