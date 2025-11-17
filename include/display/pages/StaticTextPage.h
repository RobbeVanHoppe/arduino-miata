#pragma once

#include "display/DisplayPage.h"

class StaticTextPage : public DisplayPage {
public:
    StaticTextPage(String title, String body,
                   uint16_t titleColor = 0xFFFF,
                   uint16_t bodyColor = 0x07E0,
                   uint16_t backgroundColor = 0x0000);

    void setTitle(const String &title);
    void setBody(const String &body);

    void render(Adafruit_GC9A01A &display) override;

private:
    String _title;
    String _body;
    uint16_t _titleColor;
    uint16_t _bodyColor;
    uint16_t _backgroundColor;
};
