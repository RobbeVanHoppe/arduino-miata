#pragma once

#include <Arduino.h>

#include "Config.hpp"
#include "SensorReadings.hpp"

class SensorService {
public:
    void begin();
    void sampleAnalogSensors(SensorReadings& readings);
    void sampleRpm(SensorReadings& readings);
    static void IRAM_ATTR handleTachPulseISR();

private:
    float analogReadVoltage(uint8_t pin) const;
    float mapSensorValue(float voltage, float vMin, float vMax, float outMin, float outMax) const;
    float filterValue(float previous, float newValue) const;

    static volatile uint32_t tachPulseCount_;
    static portMUX_TYPE tachMux_;
};
