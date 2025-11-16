#pragma once

#include <Arduino.h>

#include "Config.hpp"

class HardwareController {
public:
    void begin();
    void setLights(bool on);
    void setWindows(bool up);
};
