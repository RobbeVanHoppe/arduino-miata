#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "main.h"
#include <math.h>

#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"
#include "display/pages/TachPage.h"
#include "display/pages/WaterTempPage.h"
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
StaticTextPage startupPage("Miata", "Booting...");
WaterTempPage waterPage;
TachPage tachPage;

namespace {
constexpr int kWaterTempPin = 34;
constexpr int kTachSignalPin = 35;

constexpr float kAdcReferenceVoltage = 3.3f;
constexpr int kAdcResolution = 4095;
constexpr float kWaterPullupResistorOhms = 4700.0f;

constexpr uint32_t kWaterSampleIntervalMs = 500;
constexpr uint8_t kWaterSamples = 16;
constexpr float kWaterTempChangeThresholdF = 0.5f;

constexpr uint32_t kTachUpdateIntervalMs = 250;
constexpr float kTachPulsesPerRevolution = 2.0f;  // Miata 4-cylinder ignition
constexpr float kTachChangeThresholdRpm = 25.0f;

constexpr uint32_t kDataPageCycleMs = 8000;
constexpr size_t kWaterPageIndex = 1;  // after the startup page
constexpr size_t kTachPageIndex = 2;

struct WaterTempPoint {
    float tempF;
    float resistanceOhms;
};

const WaterTempPoint kMiataTempCurve[] = {
        {  32.0f, 5200.0f },
        {  68.0f, 2300.0f },
        { 104.0f, 1200.0f },
        { 140.0f,  600.0f },
        { 176.0f,  300.0f },
        { 212.0f,  180.0f },
};

volatile uint32_t g_tachPulseCount = 0;
portMUX_TYPE g_tachMux = portMUX_INITIALIZER_UNLOCKED;

float g_lastWaterTempF = NAN;
float g_lastRpm = 0.0f;
uint32_t g_lastWaterSample = 0;
uint32_t g_lastTachUpdate = 0;
uint32_t g_lastPageSwitch = 0;
size_t g_currentDataPage = 0;

void IRAM_ATTR onTachPulse() {
    portENTER_CRITICAL_ISR(&g_tachMux);
    g_tachPulseCount++;
    portEXIT_CRITICAL_ISR(&g_tachMux);
}

float interpolateWaterTemp(float resistance) {
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

bool readWaterTemp(float &outTempF) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < kWaterSamples; ++i) {
        sum += analogRead(kWaterTempPin);
        delayMicroseconds(150);
    }
    const float average = static_cast<float>(sum) / kWaterSamples;
    const float voltage = (average / static_cast<float>(kAdcResolution)) * kAdcReferenceVoltage;
    if (voltage <= 0.05f || voltage >= (kAdcReferenceVoltage - 0.05f)) {
        return false;
    }
    const float sensorResistance = (voltage * kWaterPullupResistorOhms) / (kAdcReferenceVoltage - voltage);
    if (sensorResistance <= 0.0f) {
        return false;
    }
    outTempF = interpolateWaterTemp(sensorResistance);
    return true;
}

String describeWaterStatus(float tempF) {
    if (tempF < 150.0f) {
        return F("Warming up");
    }
    if (tempF < 205.0f) {
        return F("Normal range");
    }
    return F("Hot!");
}

void updateWaterPage() {
    const uint32_t now = millis();
    if ((now - g_lastWaterSample) < kWaterSampleIntervalMs) {
        return;
    }
    g_lastWaterSample = now;

    float tempF = NAN;
    if (!readWaterTemp(tempF)) {
        waterPage.setStatusMessage(F("Sensor error"));
        displayManager.requestRefresh();
        return;
    }

    if (isnan(g_lastWaterTempF) || fabsf(tempF - g_lastWaterTempF) >= kWaterTempChangeThresholdF) {
        g_lastWaterTempF = tempF;
        waterPage.setWaterTemp(tempF);
        waterPage.setStatusMessage(describeWaterStatus(tempF));
        displayManager.requestRefresh();
    }
}

void updateTachPage() {
    const uint32_t now = millis();
    if ((now - g_lastTachUpdate) < kTachUpdateIntervalMs) {
        return;
    }
    const uint32_t elapsed = now - g_lastTachUpdate;
    g_lastTachUpdate = now;
    uint32_t pulses = 0;
    portENTER_CRITICAL(&g_tachMux);
    pulses = g_tachPulseCount;
    g_tachPulseCount = 0;
    portEXIT_CRITICAL(&g_tachMux);

    float rpm = 0.0f;
    if (elapsed > 0 && pulses > 0) {
        const float intervalSeconds = static_cast<float>(elapsed) / 1000.0f;
        const float pulseRate = pulses / intervalSeconds;
        rpm = (pulseRate * 60.0f) / kTachPulsesPerRevolution;
    }

    if (fabsf(rpm - g_lastRpm) >= kTachChangeThresholdRpm) {
        g_lastRpm = rpm;
        tachPage.setRpm(rpm);
        if (rpm < 100.0f) {
            tachPage.setStatusMessage(F("Engine off"));
        } else if (rpm < 1200.0f) {
            tachPage.setStatusMessage(F("Idle"));
        } else {
            tachPage.setStatusMessage(F("Running"));
        }
        displayManager.requestRefresh();
    }
}

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

void updateSensors() {
    updateWaterPage();
    updateTachPage();
    cycleDataPages();
}
}

void setup() {
    Serial.begin(115200);
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

    appendStartupMessage(F("Powering display..."), 1000);

    if (!startupLog.isEmpty()) {
        startupLog += '\n';
    }
    startupLog += F("Starting BLE server...");
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

    pCharacteristic->setCallbacks(new MyCustomCallbacks(waterPage, displayManager));
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

    appendStartupMessage(F("Waiting for client..."), 3000);

    waterPage.setWaterTemp(185.0f);
    waterPage.setStatusMessage("Awaiting client");
    tachPage.setStatusMessage("Awaiting tach signal");
    displayManager.showPage(kWaterPageIndex);
    displayManager.requestRefresh();
    displayManager.loop();

    pinMode(kWaterTempPin, INPUT);
    pinMode(kTachSignalPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(kTachSignalPin), onTachPulse, RISING);
}

void loop() {
    updateSensors();
    displayManager.loop();
    delay(50);
}
