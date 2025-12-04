#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "main.h"
#include <math.h>

#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"
#include "display/pages/TachPage.h"
#include "display/pages/WaterTempPage.h"
#include "sensors/TachSensor.h"
#include "sensors/WaterSensor.h"
#include "TM1638/TM1638LedAndKey.h"
#include "myCustomCallbacks.h"
#include "myServerCallbacks.h"

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;

namespace {
DisplayConfig makeDisplayConfig() {
    DisplayConfig cfg;
    cfg.csPin = 5;
    cfg.dcPin = 16;
    cfg.rstPin = 17;
    cfg.backlightPin = 4;
    cfg.rotation = 0;
    cfg.backgroundColor = 0x0000;
    cfg.refreshIntervalMs = 0;  // redraw only when something changes
    cfg.width = 240;
    cfg.height = 240;
    return cfg;
}
}

DisplayManager displayManager(makeDisplayConfig());
StaticTextPage startupPage("Miata", "Booting");
WaterTempPage waterPage;
TachPage tachPage;
constexpr uint32_t kStatusOverlayDurationMs = 2000;

namespace {
constexpr int kWaterTempPin = 34;
constexpr int kTachSignalPin = 35;

constexpr float kAdcReferenceVoltage = 3.3f;
constexpr int kAdcResolution = 4095;
constexpr float kWaterPullupResistorOhms = 4700.0f;

constexpr uint32_t kWaterSampleIntervalMs = 500;
constexpr uint8_t kWaterSamples = 16;
constexpr float kWaterTempChangeThresholdC = 0.5f;

constexpr uint32_t kTachUpdateIntervalMs = 250;
constexpr float kTachPulsesPerRevolution = 2.0f;  // Miata 4-cylinder ignition
constexpr float kTachChangeThresholdRpm = 25.0f;
constexpr uint32_t kTachMinPulseIntervalMicros = 2000;

constexpr uint32_t kDataPageCycleMs = 8000;
constexpr size_t kWaterPageIndex = 1;  // after the startup page
constexpr size_t kTachPageIndex = 2;

uint32_t g_lastPageSwitch = 0;
size_t g_currentDataPage = 0;
bool g_lowPowerMode = false;
//TM1638LedAndKeyModule g_ledKeyModule({});
uint8_t g_lastKeyState = 0;

void cycleDataPages() {
    if (displayManager.display() == nullptr) {
        return;
    }
    const uint32_t now = millis();
    if ((now - g_lastPageSwitch) < kDataPageCycleMs) {
        return;
    }
    g_lastPageSwitch = now;
    g_currentDataPage = (g_currentDataPage + 1) % 2;
    const size_t pageIndex = g_currentDataPage == 0 ? kWaterPageIndex : kTachPageIndex;
    displayManager.showPage(pageIndex);
    displayManager.requestRefresh();
}

WaterSensor waterSensor({
        .analogPin = kWaterTempPin,
        .referenceVoltage = kAdcReferenceVoltage,
        .adcResolution = kAdcResolution,
        .pullupResistorOhms = kWaterPullupResistorOhms,
        .sampleIntervalMs = kWaterSampleIntervalMs,
        .samples = kWaterSamples,
        .changeThresholdC = kWaterTempChangeThresholdC,
}, waterPage, displayManager);

TachSensor tachSensor({
        .signalPin = kTachSignalPin,
        .updateIntervalMs = kTachUpdateIntervalMs,
        .pulsesPerRevolution = kTachPulsesPerRevolution,
        .changeThresholdRpm = kTachChangeThresholdRpm,
        .minPulseIntervalMicros = kTachMinPulseIntervalMicros,
}, tachPage, displayManager);

void updateSensors() {
    waterSensor.update();
    tachSensor.update();
//    cycleDataPages();
}

//void updateInputs() {
//    const uint8_t keys = g_ledKeyModule.readKeys();
//    const uint8_t newlyPressed = keys & ~g_lastKeyState;
//    g_lastKeyState = keys;
//    Serial.print(newlyPressed);
//
//    if (newlyPressed & 0x01) {
//        displayManager.previousPage();
//        displayManager.requestRefresh();
//    }
//    if (newlyPressed & 0x02) {
//        displayManager.nextPage();
//        displayManager.requestRefresh();
//    }
//}
}

void showTransientStatusMessage(const String &message) {
    if (!displayManager.isReady()) {
        return;
    }
    if (displayManager.isSuspended()) {
        Serial.print(F("Status (suspended): "));
        Serial.println(message);
        return;
    }
    displayManager.showTransientMessage(message, kStatusOverlayDurationMs);
}

void showTransientStatusMessage(const __FlashStringHelper *message) {
    showTransientStatusMessage(String(message));
}

void enterLowPowerMode() {
    if (g_lowPowerMode) {
        return;
    }
    Serial.println(F("Entering low power mode"));
    g_lowPowerMode = true;
    waterSensor.setEnabled(false);
    tachSensor.setEnabled(false);
    displayManager.setSuspended(true);
}

void exitLowPowerMode() {
    if (!g_lowPowerMode) {
        return;
    }
    Serial.println(F("Leaving low power mode"));
    g_lowPowerMode = false;
    displayManager.setSuspended(false);
    waterSensor.setEnabled(true);
    tachSensor.setEnabled(true);
    showTransientStatusMessage(F("Awake"));
}

bool isLowPowerMode() {
    return g_lowPowerMode;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

//    g_ledKeyModule.begin();

    displayManager.addPage(&startupPage);
    displayManager.addPage(&waterPage);
    displayManager.addPage(&tachPage);
    displayManager.begin();
    displayManager.requestRefresh();
    displayManager.loop();

    String startupLog;
    auto appendStartupMessage = [&](const __FlashStringHelper *message, uint32_t pauseMs) {
        if (!startupLog.isEmpty()) {
            startupLog += '\n';
        }
        startupLog += String(message);
        startupPage.setBody(startupLog);
        displayManager.requestRefresh();
        displayManager.loop();
        if (pauseMs > 0) {
            delay(pauseMs);
        }
    };

    appendStartupMessage(F("Powering display"), 1000);

    if (!startupLog.isEmpty()) {
        startupLog += '\n';
    }
    startupLog += F("Starting BLE");
    startupPage.setBody(startupLog);
    displayManager.requestRefresh();
    displayManager.loop();

    Serial.println("Starting BLE Server...");

    BLEDevice::init("ESP32-Control");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(waterPage, displayManager));

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new MyCustomCallbacks());
    pCharacteristic->setValue("Hello");

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    delay(1000);  // keep the startup screen visible for the full 5 seconds

    Serial.println("BLE device is ready, advertising as 'ESP32-Control'");

    appendStartupMessage(F("Awaiting client"), 3000);

    waterPage.setWaterTemp(85.0f);
    waterPage.setStatusMessage("Awaiting client");
    tachPage.setStatusMessage("Awaiting tach signal");
    displayManager.showPage(kTachPageIndex);
    displayManager.requestRefresh();
    displayManager.loop();

    waterSensor.begin();
    tachSensor.begin();
}

void loop() {
    updateSensors();
    updateInputs();
    displayManager.loop();
    delay(50);
}
