#include "sensors/WaterSensor.h"

#include <math.h>

#include "display/DisplayManager.h"
#include "display/pages/WaterTempPage.h"

namespace {
constexpr WaterSensor::WaterTempPoint kMiataTempCurve[] = {
        {  32.0f, 5200.0f },
        {  68.0f, 2300.0f },
        { 104.0f, 1200.0f },
        { 140.0f,  600.0f },
        { 176.0f,  300.0f },
        { 212.0f,  180.0f },
};
}

WaterSensor::WaterSensor(const Config &config, WaterTempPage &page, DisplayManager &displayManager)
        : config_(config), page_(page), displayManager_(displayManager) {}

void WaterSensor::begin() {
    pinMode(config_.analogPin, INPUT);
    lastSampleMs_ = 0;
    lastTempF_ = NAN;
}

void WaterSensor::update() {
    const uint32_t now = millis();
    if ((now - lastSampleMs_) < config_.sampleIntervalMs) {
        return;
    }
    lastSampleMs_ = now;

    float tempF = NAN;
    if (!readWaterTemp(tempF)) {
        page_.setStatusMessage(F("Sensor error"));
        displayManager_.requestRefresh();
        return;
    }

    if (isnan(lastTempF_) || fabsf(tempF - lastTempF_) >= config_.changeThresholdF) {
        lastTempF_ = tempF;
        page_.setWaterTemp(tempF);
        page_.setStatusMessage(describeWaterStatus(tempF));
        displayManager_.requestRefresh();
    }
}

bool WaterSensor::readWaterTemp(float &outTempF) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < config_.samples; ++i) {
        sum += analogRead(config_.analogPin);
        delayMicroseconds(150);
    }
    const float average = static_cast<float>(sum) / config_.samples;
    const float voltage = (average / static_cast<float>(config_.adcResolution)) * config_.referenceVoltage;
    if (voltage <= 0.05f || voltage >= (config_.referenceVoltage - 0.05f)) {
        return false;
    }
    const float sensorResistance = (voltage * config_.pullupResistorOhms) / (config_.referenceVoltage - voltage);
    if (sensorResistance <= 0.0f) {
        return false;
    }
    outTempF = interpolateWaterTemp(sensorResistance);
    return true;
}

float WaterSensor::interpolateWaterTemp(float resistance) {
    constexpr size_t kPoints = sizeof(kMiataTempCurve) / sizeof(kMiataTempCurve[0]);
    if (resistance >= kMiataTempCurve[0].resistanceOhms) {
        return kMiataTempCurve[0].tempF;
    }
    if (resistance <= kMiataTempCurve[kPoints - 1].resistanceOhms) {
        return kMiataTempCurve[kPoints - 1].tempF;
    }
    for (size_t i = 1; i < kPoints; ++i) {
        const auto &prev = kMiataTempCurve[i - 1];
        const auto &curr = kMiataTempCurve[i];
        if (resistance >= curr.resistanceOhms) {
            const float span = prev.resistanceOhms - curr.resistanceOhms;
            const float offset = resistance - curr.resistanceOhms;
            const float fraction = offset / span;
            return curr.tempF + (prev.tempF - curr.tempF) * fraction;
        }
    }
    return kMiataTempCurve[kPoints - 1].tempF;
}

String WaterSensor::describeWaterStatus(float tempF) {
    if (tempF < 150.0f) {
        return F("Warming up");
    }
    if (tempF < 205.0f) {
        return F("Normal range");
    }
    return F("Hot!");
}
