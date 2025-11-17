#include "display/pages/StaticTextPage.h"

#include <utility>

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

    display.setTextWrap(true);
    display.setTextSize(2);
    display.setTextColor(_titleColor);
    display.setCursor(20, 30);
    display.println(_title);

    display.setTextSize(3);
    display.setTextColor(_bodyColor);
    display.setCursor(20, 120);
    display.println(_body);
}
