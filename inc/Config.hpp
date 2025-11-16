#pragma once

#include <Arduino.h>

namespace Config {

// GPIO configuration
constexpr int LIGHTS_PIN = 2;
constexpr int WINDOWS_PIN = 3;
constexpr int TACH_PIN = 26;
constexpr int WATER_TEMP_PIN = 34;
constexpr int OIL_PRESSURE_PIN = 35;
constexpr int HANDBRAKE_PIN = 27;
constexpr int TFT_CS_PIN = 5;
constexpr int TFT_DC_PIN = 16;
constexpr int TFT_RST_PIN = 17;
constexpr int TFT_BACKLIGHT_PIN = 4;
constexpr int TFT_SDA_PIN = 23;
constexpr int TFT_SCL_PIN = 18;

// Display geometry
constexpr int16_t DISPLAY_WIDTH = 240;
constexpr int16_t DISPLAY_HEIGHT = 240;
constexpr int16_t DISPLAY_CENTER_X = DISPLAY_WIDTH / 2;
constexpr int16_t DISPLAY_CENTER_Y = DISPLAY_HEIGHT / 2;
constexpr int16_t DISPLAY_RADIUS = DISPLAY_WIDTH / 2 - 5;
constexpr int16_t VALUE_TEXT_Y = DISPLAY_CENTER_Y - 5;
constexpr uint8_t TITLE_TEXT_SIZE = 2;
constexpr uint8_t VALUE_TEXT_SIZE = 4;
constexpr uint8_t UNIT_TEXT_SIZE = 2;
constexpr int16_t VALUE_TEXT_PADDING = 8;

// Analog sensor calibration
constexpr float ANALOG_REFERENCE_V = 3.3f;
constexpr int ADC_RESOLUTION = 4095;
constexpr float WATER_TEMP_SENSOR_V_MIN = 0.5f;
constexpr float WATER_TEMP_SENSOR_V_MAX = 4.5f;
constexpr float WATER_TEMP_MIN_C = 40.0f;
constexpr float WATER_TEMP_MAX_C = 140.0f;
constexpr float OIL_PRESSURE_SENSOR_V_MIN = 0.5f;
constexpr float OIL_PRESSURE_SENSOR_V_MAX = 4.5f;
constexpr float OIL_PRESSURE_MIN_PSI = 0.0f;
constexpr float OIL_PRESSURE_MAX_PSI = 100.0f;
constexpr uint8_t HANDBRAKE_ACTIVE_LEVEL = LOW;

// Tachometer calibration
constexpr float TACH_PULSES_PER_REV = 2.0f;

// Timing
constexpr unsigned long SENSOR_SAMPLE_INTERVAL_MS = 200;
constexpr unsigned long RPM_SAMPLE_INTERVAL_MS = 500;
constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 250;
constexpr unsigned long BLE_NOTIFY_INTERVAL_MS = 500;
constexpr float SENSOR_FILTER_ALPHA = 0.2f;

// BLE UUIDs
constexpr const char* SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* COMMAND_CHARACTERISTIC_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* TELEMETRY_CHARACTERISTIC_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

}  // namespace Config
