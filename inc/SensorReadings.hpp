#pragma once

#include <cstdint>
#include <cmath>

struct SensorReadings {
    float waterTempC = NAN;
    float oilPressurePsi = NAN;
    uint16_t rpm = 0;
    bool handbrakeEngaged = false;
};
