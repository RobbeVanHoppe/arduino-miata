#pragma once

#include <Arduino.h>

class DisplayManager;
class WaterTempPage;

class WaterSensor {
public:
    struct Config {
        int analogPin;
        float referenceVoltage;
        int adcResolution;
        float pullupResistorOhms;
        uint32_t sampleIntervalMs;
        uint8_t samples;
        float changeThresholdF;
    };

    WaterSensor(const Config &config, WaterTempPage &page, DisplayManager &displayManager);

    void begin();
    void update();

    float lastTempF() const { return lastTempF_; }

private:
    struct WaterTempPoint {
        float tempF;
        float resistanceOhms;
    };

    bool readWaterTemp(float &outTempF);
    static float interpolateWaterTemp(float resistance);
    static String describeWaterStatus(float tempF);

    const Config config_;
    WaterTempPage &page_;
    DisplayManager &displayManager_;

    uint32_t lastSampleMs_ = 0;
    float lastTempF_ = NAN;
};
