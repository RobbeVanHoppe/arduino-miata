#include "esp32_dash/sensors/WaterSensor.h"

#include <math.h>

#include "esp32_dash/display/DisplayManager.h"
#include "esp32_dash/display/pages/WaterTempPage.h"

namespace {
constexpr WaterSensor::WaterTempPoint kMiataTempCurve[] = {
        {   0.0f, 5200.0f },
        {  20.0f, 2300.0f },
        {  40.0f, 1200.0f },
        {  60.0f,  600.0f },
        {  80.0f,  300.0f },
        { 100.0f,  180.0f },
};
}

WaterSensor::WaterSensor(const Config &config, WaterTempPage &page, DisplayManager &displayManager)
        : config_(config), page_(page), displayManager_(displayManager) {}

void WaterSensor::begin() {
    pinMode(config_.analogPin, INPUT);
    lastSampleMs_ = 0;
    lastTempC_ = NAN;
    enabled_ = true;
}

void WaterSensor::update() {
    if (!enabled_) {
        return;
    }
    const uint32_t now = millis();
    if ((now - lastSampleMs_) < config_.sampleIntervalMs) {
        return;
    }
    lastSampleMs_ = now;

    float tempC = NAN;
    if (!readWaterTemp(tempC)) {
        page_.setStatusMessage(F("Sensor error"));
        displayManager_.requestRefresh();
        return;
    }

    if (isnan(lastTempC_) || fabsf(tempC - lastTempC_) >= config_.changeThresholdC) {
        lastTempC_ = tempC;
        page_.setWaterTemp(tempC);
        page_.setStatusMessage(describeWaterStatus(tempC));
        displayManager_.requestRefresh();
    }
}

bool WaterSensor::readWaterTemp(float &outTempC) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < config_.samples; ++i) {
        sum += analogRead(config_.analogPin);
        delayMicroseconds(150);
    }
    const float average = static_cast<float>(sum) / config_.samples;
    if (average <= 1.0f || average >= (static_cast<float>(config_.adcResolution) - 1.0f)) {
        return false;
    }
    const float voltage = (average / static_cast<float>(config_.adcResolution)) * config_.referenceVoltage;
    const float sensorResistance = (voltage * config_.pullupResistorOhms) / (config_.referenceVoltage - voltage);
    if (sensorResistance <= 0.0f) {
        return false;
    }
    outTempC = interpolateWaterTemp(sensorResistance);
    return true;
}

float WaterSensor::interpolateWaterTemp(float resistance) {
    constexpr size_t kPoints = sizeof(kMiataTempCurve) / sizeof(kMiataTempCurve[0]);
    if (resistance >= kMiataTempCurve[0].resistanceOhms) {
        return kMiataTempCurve[0].tempC;
    }
    if (resistance <= kMiataTempCurve[kPoints - 1].resistanceOhms) {
        return kMiataTempCurve[kPoints - 1].tempC;
    }
    for (size_t i = 1; i < kPoints; ++i) {
        const auto &prev = kMiataTempCurve[i - 1];
        const auto &curr = kMiataTempCurve[i];
        if (resistance >= curr.resistanceOhms) {
            const float span = prev.resistanceOhms - curr.resistanceOhms;
            const float offset = resistance - curr.resistanceOhms;
            const float fraction = offset / span;
            return curr.tempC + (prev.tempC - curr.tempC) * fraction;
        }
    }
    return kMiataTempCurve[kPoints - 1].tempC;
}

String WaterSensor::describeWaterStatus(float tempC) {
    if (tempC < 80.0f) {
        return F("Warming up");
    }
    if (tempC < 105.0f) {
        return F("");
    }
    return F("Hot!");
}

void WaterSensor::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        page_.setStatusMessage(F("Sleeping"));
    } else if (!isnan(lastTempC_)) {
        page_.setStatusMessage(describeWaterStatus(lastTempC_));
    }
    displayManager_.requestRefresh();
}
