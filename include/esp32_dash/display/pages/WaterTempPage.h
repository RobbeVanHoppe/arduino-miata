#pragma once

#include "esp32_dash/display/DisplayPage.h"

class WaterTempPage : public DisplayPage {
public:
    WaterTempPage();

    void setTitle(const String &title);
    void setWaterTemp(float tempC);
    void setStatusMessage(const String &status);

    void onEnter(Adafruit_GC9A01A &display) override;

    void render(Adafruit_GC9A01A &display) override;

private:
    void drawBaseLayout(Adafruit_GC9A01A &display);

    String _title;
    float _waterTempC;
    String _statusMessage;
    uint16_t _backgroundColor;
    uint16_t _titleColor;
    uint16_t _tempColor;
    uint16_t _statusColor;
    bool _layoutDirty;
};
