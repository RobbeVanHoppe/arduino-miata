#pragma once

#include "DisplayPage.h"

class WaterTempPage : public DisplayPage {
public:
    WaterTempPage();

    void setWaterTemp(float tempC);
    void onEnter(Adafruit_GC9A01A &display) override;
    void render(Adafruit_GC9A01A &display) override;

private:
    void drawBaseLayout(Adafruit_GC9A01A &display) override;

    float _waterTempC;
    uint16_t _tempColor;
    uint16_t _statusColor;
    bool _layoutDirty;
};
