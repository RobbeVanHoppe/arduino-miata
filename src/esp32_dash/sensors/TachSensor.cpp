#include "esp32_dash/sensors/TachSensor.h"

#include <math.h>

#include "esp32_dash/display/DisplayManager.h"
#include "esp32_dash/display/pages/TachPage.h"

TachSensor *TachSensor::instance_ = nullptr;

TachSensor::TachSensor(const Config &config, TachPage &page, DisplayManager &displayManager)
        : config_(config), page_(page), displayManager_(displayManager) {}

void TachSensor::begin() {
    pinMode(config_.signalPin, INPUT);
    instance_ = this;
    attachInterrupt(digitalPinToInterrupt(config_.signalPin), TachSensor::handlePulse, RISING);
    lastUpdateMs_ = 0;
    lastRpm_ = 0.0f;
    lastPulseMicros_ = 0;
    enabled_ = true;
}

void TachSensor::update() {
    if (!enabled_) {
        return;
    }
    const uint32_t now = millis();
    if ((now - lastUpdateMs_) < config_.updateIntervalMs) {
        return;
    }
    const uint32_t elapsed = now - lastUpdateMs_;
    lastUpdateMs_ = now;

    uint32_t pulses = 0;
    portENTER_CRITICAL(&mux_);
    pulses = pulseCount_;
    pulseCount_ = 0;
    portEXIT_CRITICAL(&mux_);

    float rpm = 0.0f;
    if (elapsed > 0 && pulses > 0) {
        const float intervalSeconds = static_cast<float>(elapsed) / 1000.0f;
        const float pulseRate = pulses / intervalSeconds;
        rpm = (pulseRate * 60.0f) / config_.pulsesPerRevolution;
    }

    if (fabsf(rpm - lastRpm_) >= config_.changeThresholdRpm) {
        lastRpm_ = rpm;
        page_.setRpm(rpm);
        if (rpm < 100.0f) {
            page_.setStatusMessage(F("Engine off"));
        } else if (rpm < 1200.0f) {
            page_.setStatusMessage(F("Idle"));
        } else if (rpm > 5500.0f) {
            page_.setStatusMessage(F("Shift pls"));
        } else {
            page_.setStatusMessage("");
        }
        displayManager_.requestRefresh();
    }
}

void IRAM_ATTR TachSensor::handlePulse() {
    if (instance_ != nullptr) {
        instance_->recordPulse();
    }
}

void TachSensor::recordPulse() {
    portENTER_CRITICAL_ISR(&mux_);
    if (!enabled_) {
        portEXIT_CRITICAL_ISR(&mux_);
        return;
    }
    const uint32_t now = micros();
    if (lastPulseMicros_ != 0) {
        const uint32_t delta = now - lastPulseMicros_;
        if (delta < config_.minPulseIntervalMicros) {
            portEXIT_CRITICAL_ISR(&mux_);
            return;
        }
    }
    lastPulseMicros_ = now;
    pulseCount_++;
    portEXIT_CRITICAL_ISR(&mux_);
}

void TachSensor::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }
    portENTER_CRITICAL(&mux_);
    enabled_ = enabled;
    pulseCount_ = 0;
    lastPulseMicros_ = 0;
    portEXIT_CRITICAL(&mux_);
    if (!enabled) {
        lastRpm_ = 0.0f;
        page_.setRpm(0.0f);
        page_.setStatusMessage(F("Sleeping"));
    } else {
        page_.setStatusMessage(F("Awaiting tach signal"));
    }
    displayManager_.requestRefresh();
}
