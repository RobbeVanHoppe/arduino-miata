#include "esp32_dash/display/pages/WaterTempPage.h"

namespace {
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
}

WaterTempPage::WaterTempPage()
        : DisplayPage(F("Water Temp"), 0x0000, 0xFFFF, F("Awaiting tach signal")),
          _waterTempC(85.0f),
          _tempColor(0x07E0),
          _statusColor(0xFFE0),
          _layoutDirty(true) {}


void WaterTempPage::setWaterTemp(float tempC) {
    _waterTempC = tempC;
}

void WaterTempPage::onEnter(Adafruit_GC9A01A &display) {
    (void) display;
    _layoutDirty = true;
}

void WaterTempPage::drawBaseLayout(Adafruit_GC9A01A &display) {
    display.fillScreen(_backgroundColor);
    display.setTextWrap(false);
    display.setTextColor(_titleColor, _backgroundColor);
    drawCenteredText(display, _title, kTitleY, 3);
    _layoutDirty = false;
}

void WaterTempPage::render(Adafruit_GC9A01A &display) {
    if (_layoutDirty) {
        drawBaseLayout(display);
    } else {
        display.setTextWrap(false);
    }

    display.setTextColor(_tempColor, _backgroundColor);
    const String tempText = String(static_cast<int>(_waterTempC)) + F(" C");
    const int16_t tempY = (display.height() / 2) - 30;
    clearTextBand(display, tempY, 6, _backgroundColor);
    drawCenteredText(display, tempText, tempY, 6);

    display.setTextColor(_statusColor, _backgroundColor);
    const int16_t statusY = display.height() - kSafeMargin - kStatusYOffset;
    clearTextBand(display, statusY, 2, _backgroundColor);
    drawCenteredText(display, _statusMessage, statusY, 2);
}
