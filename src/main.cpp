#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"
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
    cfg.refreshIntervalMs = 250;
    cfg.width = 240;
    cfg.height = 240;
    return cfg;
}
}

DisplayManager displayManager(makeDisplayConfig());
StaticTextPage statusPage("BLE Status", "Booting...");

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

    displayManager.begin();
    displayManager.addPage(&statusPage);
    statusPage.setBody("Awaiting client");
    displayManager.requestRefresh();

    Serial.println("Starting BLE Server...");

    BLEDevice::init("ESP32-Control");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(statusPage, displayManager));

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new MyCustomCallbacks(statusPage, displayManager));
    pCharacteristic->setValue("Hello");

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE device is ready, advertising as 'ESP32-Control'");
    statusPage.setBody("Client set up");
    displayManager.requestRefresh();
}

void loop() {
    displayManager.loop();
    delay(50);
}
