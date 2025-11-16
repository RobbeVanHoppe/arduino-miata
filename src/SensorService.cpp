#include "SensorService.hpp"

#include <cmath>

volatile uint32_t SensorService::tachPulseCount_ = 0;
portMUX_TYPE SensorService::tachMux_ = portMUX_INITIALIZER_UNLOCKED;

void SensorService::begin() {
    pinMode(Config::HANDBRAKE_PIN, INPUT_PULLUP);
    pinMode(Config::TACH_PIN, INPUT_PULLUP);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    attachInterrupt(digitalPinToInterrupt(Config::TACH_PIN), SensorService::handleTachPulseISR, RISING);
}

void SensorService::sampleAnalogSensors(SensorReadings& readings) {
    const float waterVoltage = analogReadVoltage(Config::WATER_TEMP_PIN);
    const float waterTemp = mapSensorValue(waterVoltage,
                                           Config::WATER_TEMP_SENSOR_V_MIN,
                                           Config::WATER_TEMP_SENSOR_V_MAX,
                                           Config::WATER_TEMP_MIN_C,
                                           Config::WATER_TEMP_MAX_C);
    readings.waterTempC = filterValue(readings.waterTempC, waterTemp);

    const float oilVoltage = analogReadVoltage(Config::OIL_PRESSURE_PIN);
    const float oilPressure = mapSensorValue(oilVoltage,
                                             Config::OIL_PRESSURE_SENSOR_V_MIN,
                                             Config::OIL_PRESSURE_SENSOR_V_MAX,
                                             Config::OIL_PRESSURE_MIN_PSI,
                                             Config::OIL_PRESSURE_MAX_PSI);
    readings.oilPressurePsi = filterValue(readings.oilPressurePsi, oilPressure);

    readings.handbrakeEngaged = digitalRead(Config::HANDBRAKE_PIN) == Config::HANDBRAKE_ACTIVE_LEVEL;
}

void SensorService::sampleRpm(SensorReadings& readings) {
    uint32_t pulses = 0;
    portENTER_CRITICAL(&tachMux_);
    pulses = tachPulseCount_;
    tachPulseCount_ = 0;
    portEXIT_CRITICAL(&tachMux_);

    if (pulses == 0 || Config::TACH_PULSES_PER_REV <= 0.0f) {
        readings.rpm = 0;
        return;
    }

    const float pulsesPerMinute = (static_cast<float>(pulses) * 60000.0f) / static_cast<float>(Config::RPM_SAMPLE_INTERVAL_MS);
    const float rpm = pulsesPerMinute / Config::TACH_PULSES_PER_REV;
    readings.rpm = static_cast<uint16_t>(rpm + 0.5f);
}

void IRAM_ATTR SensorService::handleTachPulseISR() {
    portENTER_CRITICAL_ISR(&tachMux_);
    tachPulseCount_++;
    portEXIT_CRITICAL_ISR(&tachMux_);
}

float SensorService::analogReadVoltage(uint8_t pin) const {
    uint16_t raw = analogRead(pin);
    return (static_cast<float>(raw) / static_cast<float>(Config::ADC_RESOLUTION)) * Config::ANALOG_REFERENCE_V;
}

float SensorService::mapSensorValue(float voltage, float vMin, float vMax, float outMin, float outMax) const {
    voltage = constrain(voltage, vMin, vMax);
    const float range = vMax - vMin;
    if (fabsf(range) < 0.0001f) {
        return outMin;
    }
    const float normalized = (voltage - vMin) / range;
    return outMin + normalized * (outMax - outMin);
}

float SensorService::filterValue(float previous, float newValue) const {
    if (isnan(previous)) {
        return newValue;
    }
    return previous + Config::SENSOR_FILTER_ALPHA * (newValue - previous);
}
