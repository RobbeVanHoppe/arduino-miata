#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "main.h"
#include "myCustomCallbacks.h"
#include "myServerCallbacks.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

    Serial.println("Starting BLE Server...");

    // Init BLE with device name
    BLEDevice::init("ESP32-Control");

    // Create server and set callbacks
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Create characteristic – readable + writable from your phone
    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ   |
            BLECharacteristic::PROPERTY_WRITE  |
            BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->setCallbacks(new MyCustomCallbacks());
    pCharacteristic->setValue("Hello");

    // Start service
    pService->start();

    // Start advertising so your phone can see it
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // helps iOS
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE device is ready, advertising as 'ESP32-Control'");
}

void loop() {
    // non blocking delay
    delay(1000);
}
