#pragma once

#include "display/DisplayPage.h"

class TachPage : public DisplayPage {
public:
    TachPage();

    void setTitle(const String &title);
    void setRpm(float rpm);
    void setStatusMessage(const String &status);

    void onEnter(Adafruit_GC9A01A &display) override;

    void render(Adafruit_GC9A01A &display) override;

private:
    void drawBaseLayout(Adafruit_GC9A01A &display);

    String _title;
    float _rpm;
    String _statusMessage;
    uint16_t _backgroundColor;
    uint16_t _titleColor;
    uint16_t _rpmColor;
    uint16_t _statusColor;
    bool _layoutDirty;
};
