#pragma once

#include "DisplayPage.h"


class TachPage : public DisplayPage {
public:
    TachPage();

    void setRpm(float rpm);
    void onEnter(Adafruit_GC9A01A &display) override;
    void render(Adafruit_GC9A01A &display) override;

private:
    void drawBaseLayout(Adafruit_GC9A01A &display) override;

    float _rpm;
    uint16_t _rpmColor;
    uint16_t _statusColor;
    bool _layoutDirty;
};
