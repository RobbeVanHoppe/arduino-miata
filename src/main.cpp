#include <Arduino.h>
#include <cmath>
#include <cstdio>

#include "BleService.hpp"
#include "CommandProcessor.hpp"
#include "Config.hpp"
#include "DisplayService.hpp"
#include "HardwareController.hpp"
#include "MenuPage.hpp"
#include "SensorReadings.hpp"
#include "SensorService.hpp"

namespace {

struct RpmFormatter {
    void operator()(const SensorReadings& readings, char* buffer, size_t len) const {
        snprintf(buffer, len, "%4u", readings.rpm);
    }
};

struct WaterTempFormatter {
    void operator()(const SensorReadings& readings, char* buffer, size_t len) const {
        if (isnan(readings.waterTempC)) {
            snprintf(buffer, len, "--.-");
        } else {
            snprintf(buffer, len, "%5.1f", readings.waterTempC);
        }
    }
};

struct OilPressureFormatter {
    void operator()(const SensorReadings& readings, char* buffer, size_t len) const {
        if (isnan(readings.oilPressurePsi)) {
            snprintf(buffer, len, "--.-");
        } else {
            snprintf(buffer, len, "%5.1f", readings.oilPressurePsi);
        }
    }
};

struct HandbrakeFormatter {
    void operator()(const SensorReadings& readings, char* buffer, size_t len) const {
        const char* text = readings.handbrakeEngaged ? "ENGAGED" : "Released";
        snprintf(buffer, len, "%s", text);
    }
};

const MenuPage<RpmFormatter> ENGINE_PAGE{"Engine", "rpm", RpmFormatter{}};
const MenuPage<WaterTempFormatter> WATER_PAGE{"Water", "\xB0" "C", WaterTempFormatter{}};
const MenuPage<OilPressureFormatter> OIL_PAGE{"Oil", "psi", OilPressureFormatter{}};
const MenuPage<HandbrakeFormatter> HANDBRAKE_PAGE{"Handbrake", nullptr, HandbrakeFormatter{}};

constexpr const MenuPageBase* MENU_PAGES[] = {
    &ENGINE_PAGE,
    &WATER_PAGE,
    &OIL_PAGE,
    &HANDBRAKE_PAGE,
};

constexpr size_t MENU_PAGE_COUNT = sizeof(MENU_PAGES) / sizeof(MENU_PAGES[0]);

SensorService sensorService;
DisplayService displayService;
HardwareController hardwareController;
CommandProcessor commandProcessor(hardwareController, displayService);
BleService<CommandProcessor> bleService(commandProcessor);
SensorReadings currentReadings;

}  // namespace

void setup() {
    Serial.begin(9600);
    delay(1000);
    Serial.println("Starting Miata brain...");

    hardwareController.begin();
    sensorService.begin();
    displayService.begin();
    displayService.setPages(MENU_PAGES, MENU_PAGE_COUNT);
    displayService.update(currentReadings);
    bleService.begin();
}

void loop() {
    static unsigned long lastSensorSample = 0;
    static unsigned long lastRpmSample = 0;
    static unsigned long lastDisplayUpdate = 0;

    const unsigned long now = millis();

    if (now - lastSensorSample >= Config::SENSOR_SAMPLE_INTERVAL_MS) {
        sensorService.sampleAnalogSensors(currentReadings);
        lastSensorSample = now;
    }

    if (now - lastRpmSample >= Config::RPM_SAMPLE_INTERVAL_MS) {
        sensorService.sampleRpm(currentReadings);
        lastRpmSample = now;
    }

    if (now - lastDisplayUpdate >= Config::DISPLAY_UPDATE_INTERVAL_MS) {
        displayService.update(currentReadings);
        lastDisplayUpdate = now;
    }

    bleService.loop(currentReadings);

    delay(10);
}
