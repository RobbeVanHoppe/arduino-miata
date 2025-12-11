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
        float changeThresholdC;
    };

    struct WaterTempPoint {
        float tempC;
        float resistanceOhms;
    };

    WaterSensor(const Config &config, WaterTempPage &page, DisplayManager &displayManager);

    void begin();
    void update();
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    float lastTempC() const { return lastTempC_; }

private:


    bool readWaterTemp(float &outTempC);

#ifdef UNIT_TEST
public:
#else
private:
#endif
    static float interpolateWaterTemp(float resistance);
    static String describeWaterStatus(float tempC);

    const Config config_;
    WaterTempPage &page_;
    DisplayManager &displayManager_;

    uint32_t lastSampleMs_ = 0;
    float lastTempC_ = NAN;
    bool enabled_ = true;
};
