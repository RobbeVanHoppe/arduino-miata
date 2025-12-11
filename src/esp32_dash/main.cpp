#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "esp32_dash/main.h"
#include <math.h>

#include "esp32_dash/display/DisplayManager.h"
#include "esp32_dash/display/pages/StaticTextPage.h"
#include "esp32_dash/display/pages/TachPage.h"
#include "esp32_dash/display/pages/WaterTempPage.h"
#include "esp32_dash/sensors/TachSensor.h"
#include "esp32_dash/sensors/WaterSensor.h"
#include "esp32_dash/TM1638/TM1638LedAndKey.h"
#include "esp32_dash/myCustomCallbacks.h"
#include "esp32_dash/myServerCallbacks.h"

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
    constexpr int cNanoRXPin = 33;
    constexpr int cNanoTXPin = 32;

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

    constexpr uint8_t TM1638_STROBE = 25;
    constexpr uint8_t TM1638_CLK = 26;
    constexpr uint8_t TM1638_DATA = 27;
    uint8_t g_lastTmButtons = 0;

    uint32_t g_lastPageSwitch = 0;
    size_t g_currentDataPage = 0;
    bool g_lowPowerMode = false;
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

TM1638LedAndKeyModule tm1638(TM1638_STROBE, TM1638_CLK, TM1638_DATA);

HardwareSerial nanoSerial(2);

void updateSensors() {
    waterSensor.update();
    tachSensor.update();
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

void handleTm1638Buttons() {
    uint8_t buttons = tm1638.readButtons();

    // Rising-edge detection (only trigger once per press)
    uint8_t pressed = buttons & ~g_lastTmButtons;
    g_lastTmButtons = buttons;

    if (!pressed) return;  // nothing new pressed

    // Each bit = one button
    // Bit 0 → Button 1 (left)
    // Bit 7 → Button 8 (right)

    switch (pressed) {

        case 0x01:  // Button 1
            Serial.println("Button 1 pressed");
            displayManager.previousPage();
            break;

        case 0x02:  // Button 2
            Serial.println("Button 2 pressed");
            displayManager.nextPage();
            break;

        case 0x04:  // Button 3
            Serial.println("Button 3 pressed");
            break;

        case 0x08:  // Button 4
            Serial.println("Button 4 pressed");
            break;

        case 0x10:  // Button 5
            Serial.println("Button 5 pressed");
            break;

        case 0x20:  // Button 6
            Serial.println("Button 6 pressed");
            break;

        case 0x40:  // Button 7
            Serial.println("Button 7 pressed");
            break;

        case 0x80:  // Button 8
            Serial.println("Button 8 pressed");
            break;

        default:
            Serial.println("Multiple buttons pressed");
            break;
    }

    // Optional: light the matching LED for feedback
    for (uint8_t i = 0; i < 8; i++) {
        tm1638.setLed(0, i);
    }
    tm1638.setLed(1, g_currentDataPage);
}


void setup() {
    Serial.begin(115200);
    nanoSerial.begin(115200, SERIAL_8N1, cNanoRXPin, cNanoTXPin);

    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

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
    tm1638.begin();
    tm1638.setLed(1, 0);
}

void loop() {
    updateSensors();
    handleTm1638Buttons();
    displayManager.loop();

    while (nanoSerial.available()) {
        char c = nanoSerial.read();
        Serial.write(c);   // echo everything from Nano to USB serial
    }
    delay(50);
}
