#include "esp32_dash/display/pages/TachPage.h"

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

TachPage::TachPage() : DisplayPage(F("Tacho"), 0x0000, 0xFFFF, F("Awaiting tach signal")),
    _rpm(0.0f),
    _rpmColor(0xF800),
    _statusColor(0xFFE0),
    _layoutDirty(true) {}

void TachPage::setRpm(float rpm) {
    _rpm = rpm;
}



void TachPage::onEnter(Adafruit_GC9A01A &display) {
    (void) display;
    _layoutDirty = true;
}

void TachPage::drawBaseLayout(Adafruit_GC9A01A &display) {
    DisplayPage::drawBaseLayout(display);
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
