#pragma once

#include "Arduino.h"

class WaterTempPage {
public:
    void setWaterTemp(float tempC) { lastWaterTemp = tempC; }
    void setStatusMessage(const String &status) { lastStatusMessage = status; }

    float lastWaterTemp = 0.0f;
    String lastStatusMessage;
};
