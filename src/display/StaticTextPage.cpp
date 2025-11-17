#include "display/pages/StaticTextPage.h"

#include <utility>
#include <vector>

namespace {
constexpr int16_t kCircularSafeMargin = 30;
constexpr int16_t kBodyLineSpacing = 28;

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
    if (x < kCircularSafeMargin) {
        x = kCircularSafeMargin;
    }
    const int16_t maxX = display.width() - kCircularSafeMargin - static_cast<int16_t>(w);
    if (x > maxX) {
        x = maxX;
    }

    display.setCursor(x, y);
    display.print(text);
}

std::vector<String> splitLines(const String &text) {
    std::vector<String> lines;
    int32_t start = 0;
    while (start < text.length()) {
        const int32_t end = text.indexOf('\n', start);
        if (end == -1) {
            lines.emplace_back(text.substring(start));
            break;
        }
        lines.emplace_back(text.substring(start, end));
        start = end + 1;
    }
    if (lines.empty() && !text.isEmpty()) {
        lines.emplace_back(text);
    }
    return lines;
}
}

StaticTextPage::StaticTextPage(String title, String body,
                               uint16_t titleColor,
                               uint16_t bodyColor,
                               uint16_t backgroundColor)
        : _title(std::move(title)),
          _body(std::move(body)),
          _titleColor(titleColor),
          _bodyColor(bodyColor),
          _backgroundColor(backgroundColor) {}

void StaticTextPage::setTitle(const String &title) {
    _title = title;
}

void StaticTextPage::setBody(const String &body) {
    _body = body;
}

void StaticTextPage::render(Adafruit_GC9A01A &display) {
    display.fillScreen(_backgroundColor);

    display.setTextWrap(false);
    display.setTextColor(_titleColor);
    drawCenteredText(display, _title, kCircularSafeMargin + 10, 3);

    display.setTextColor(_bodyColor);
    const auto lines = splitLines(_body);
    if (lines.empty()) {
        return;
    }

    const int16_t totalHeight = static_cast<int16_t>(lines.size()) * kBodyLineSpacing;
    int16_t startY = (display.height() / 2) - (totalHeight / 2);
    const int16_t minStartY = kCircularSafeMargin + 50;
    if (startY < minStartY) {
        startY = minStartY;
    }
    const int16_t maxStartY = display.height() - kCircularSafeMargin - totalHeight;
    if (maxStartY >= minStartY && startY > maxStartY) {
        startY = maxStartY;
    }

    for (const auto &line : lines) {
        drawCenteredText(display, line, startY, 2);
        startY += kBodyLineSpacing;
    }
}
