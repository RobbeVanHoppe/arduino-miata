#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"
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

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

    displayManager.addPage(&startupPage);
    displayManager.addPage(&waterPage);
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
    displayManager.showPage(1);
    displayManager.requestRefresh();
    displayManager.loop();
}

void loop() {
    displayManager.loop();
    delay(50);
}
