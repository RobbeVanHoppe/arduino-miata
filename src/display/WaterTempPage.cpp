#include "display/pages/WaterTempPage.h"

namespace {
constexpr int16_t kSafeMargin = 24;

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
    if (x > maxX) {
        x = maxX;
    }

    display.setCursor(x, y);
    display.print(text);
}
}

WaterTempPage::WaterTempPage()
        : _title(F("Water")),
          _waterTempC(85.0f),
          _statusMessage(F("")),
          _backgroundColor(0x0000),
          _titleColor(0xFFFF),
          _tempColor(0x07E0),
          _statusColor(0xFFE0) {}

void WaterTempPage::setTitle(const String &title) {
    _title = title;
}

void WaterTempPage::setWaterTemp(float tempC) {
    _waterTempC = tempC;
}

void WaterTempPage::setStatusMessage(const String &status) {
    _statusMessage = status;
}

void WaterTempPage::render(Adafruit_GC9A01A &display) {
    display.fillScreen(_backgroundColor);
    display.setTextWrap(false);

    display.setTextColor(_titleColor);
    drawCenteredText(display, _title, kSafeMargin + 8, 3);

    display.setTextColor(_tempColor);
    const String tempText = String(static_cast<int>(_waterTempC)) + F(" C");
    drawCenteredText(display, tempText, display.height() / 2 - 30, 6);

    display.setTextColor(_statusColor);
    drawCenteredText(display, _statusMessage, display.height() - kSafeMargin - 30, 2);
}
