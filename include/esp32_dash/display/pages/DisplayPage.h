#pragma once

#include <Arduino.h>

#include <utility>
#include "Adafruit_GC9A01A.h"

/**
 * Base interface for every drawable page on the GC9A01 display.
 *
 * Extend this class to implement custom pages. Override render to draw
 * your page and optionally onEnter / onExit to perform setup or cleanup
 * whenever the page becomes active or inactive.
 */

constexpr int16_t kSafeMargin = 24;
constexpr int16_t kTitleY = kSafeMargin + 8;
constexpr int16_t kStatusYOffset = 30;

class DisplayPage {
public:
    explicit DisplayPage(String title = "",
                         uint16_t backgroundColor = 0x0000,
                         uint16_t titleColor = 0xFFFF,
                         String statusMessage = "")
            : _title(std::move(title)),
              _backgroundColor(backgroundColor),
              _titleColor(titleColor),
              _statusMessage(std::move(statusMessage)) {}



    virtual void onEnter(Adafruit_GC9A01A &display) { (void) display; }

    virtual void onExit(Adafruit_GC9A01A &display) { (void) display; }

    virtual void render(Adafruit_GC9A01A &display) = 0;

    void setStatusMessage(const String &status) {
        _statusMessage = status;
    }

    void setTitle(const String &title) {
        _title = title;
    }

protected:
    uint16_t _backgroundColor = 0x0000;
    uint16_t _titleColor = 0xFFFF;
    String _title;
    String _statusMessage;
    uint16_t _bodyColor;


    virtual void drawBaseLayout(Adafruit_GC9A01A &display) {
        display.fillScreen(_backgroundColor);
        display.setTextWrap(false);
        display.setTextColor(_titleColor, _backgroundColor);
        drawCenteredText(display, _title, kTitleY, 3);
    };

    static void drawCenteredText(Adafruit_GC9A01A &display,
                                 const String &text,
                                 int16_t y,
                                 uint8_t textSize) {
        if (text.isEmpty()) {
            return;
        }

        int16_t x1, y1;
        uint16_t w, h;
        display.setTextSize(textSize);
        display.getTextBounds(text.c_str(), 0, y, &x1, &y1, &w, &h);

        const int16_t centeredX = (display.width() - static_cast<int16_t>(w)) / 2;
        int16_t x = centeredX;
        if (x < kSafeMargin) {
            x = kSafeMargin;
        }
        const int16_t maxX = display.width() - kSafeMargin - static_cast<int16_t>(w);
        if (maxX < kSafeMargin) {
            x = kSafeMargin;
        } else if (x > maxX) {
            x = maxX;
        }

        display.setCursor(x, y);
        display.print(text);
    }




};