#pragma once

#include "display/DisplayPage.h"

class WaterTempPage : public DisplayPage {
public:
    WaterTempPage();

    void setTitle(const String &title);
    void setWaterTemp(float tempF);
    void setStatusMessage(const String &status);

    void render(Adafruit_GC9A01A &display) override;

private:
    String _title;
    float _waterTempF;
    String _statusMessage;
    uint16_t _backgroundColor;
    uint16_t _titleColor;
    uint16_t _tempColor;
    uint16_t _statusColor;
};
