#include "esp32_dash/display/pages/TachPage.h"

namespace {
constexpr int16_t kSafeMargin = 24;
constexpr int16_t kTitleY = kSafeMargin + 8;
constexpr int16_t kStatusYOffset = 30;

void clearTextBand(Adafruit_GC9A01A &display,
                   int16_t y,
                   uint8_t textSize,
                   uint16_t backgroundColor) {
    int16_t x1, y1;
    uint16_t w, h;
    display.setTextSize(textSize);
    display.getTextBounds("88", 0, y, &x1, &y1, &w, &h);

    int16_t top = y1;
    int16_t height = static_cast<int16_t>(h);
    if (top < 0) {
        height += top;
        top = 0;
    }
    if (height <= 0) {
        return;
    }

    const int16_t width = display.width() - (kSafeMargin * 2);
    if (width <= 0) {
        return;
    }

    display.fillRect(kSafeMargin, top, width, height, backgroundColor);
}

void drawCenteredText(Adafruit_GC9A01A &display,
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
}

TachPage::TachPage()
        : _title(F("Tacho")),
          _rpm(0.0f),
          _statusMessage(F("Awaiting tach signal")),
          _backgroundColor(0x0000),
          _titleColor(0xFFFF),
          _rpmColor(0xF800),
          _statusColor(0xFFE0),
          _layoutDirty(true) {}

void TachPage::setTitle(const String &title) {
    _title = title;
    _layoutDirty = true;
}

void TachPage::setRpm(float rpm) {
    _rpm = rpm;
}

void TachPage::setStatusMessage(const String &status) {
    _statusMessage = status;
}

void TachPage::onEnter(Adafruit_GC9A01A &display) {
    (void) display;
    _layoutDirty = true;
}

void TachPage::drawBaseLayout(Adafruit_GC9A01A &display) {
    display.fillScreen(_backgroundColor);
    display.setTextWrap(false);
    display.setTextColor(_titleColor, _backgroundColor);
    drawCenteredText(display, _title, kTitleY, 3);
    _layoutDirty = false;
}

void TachPage::render(Adafruit_GC9A01A &display) {
    if (_layoutDirty) {
        drawBaseLayout(display);
    } else {
        display.setTextWrap(false);
    }

    display.setTextColor(_rpmColor, _backgroundColor);
    const String rpmText = String(static_cast<int>(_rpm));
    const int16_t rpmY = (display.height() / 2) - 30;
    clearTextBand(display, rpmY, 6, _backgroundColor);
    drawCenteredText(display, rpmText, rpmY, 6);

    display.setTextColor(_statusColor, _backgroundColor);
    const int16_t statusY = display.height() - kSafeMargin - kStatusYOffset;
    clearTextBand(display, statusY, 2, _backgroundColor);
    drawCenteredText(display, _statusMessage, statusY, 2);
}
