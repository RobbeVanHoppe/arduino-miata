#pragma once

#include <Arduino.h>

class DisplayManager;
class TachPage;

class TachSensor {
public:
    struct Config {
        int signalPin;
        uint32_t updateIntervalMs;
        float pulsesPerRevolution;
        float changeThresholdRpm;
    };

    TachSensor(const Config &config, TachPage &page, DisplayManager &displayManager);

    void begin();
    void update();

    float lastRpm() const { return lastRpm_; }

private:
    static void IRAM_ATTR handlePulse();
    void recordPulse();

    const Config config_;
    TachPage &page_;
    DisplayManager &displayManager_;

    volatile uint32_t pulseCount_ = 0;
    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    uint32_t lastUpdateMs_ = 0;
    float lastRpm_ = 0.0f;

    static TachSensor *instance_;
};
