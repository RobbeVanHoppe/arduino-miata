#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

const int LIGHTS_PIN = 2;
const int WINDOWS_PIN = 3;

#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// Callback for connect / disconnect
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        Serial.println("Client disconnected");
        // Make advertising restart so you can reconnect
        pServer->getAdvertising()->start();
    }
};

// Callback for writes to the characteristic
class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() == 0) return;

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        // Very basic command parser
        if (rxValue == "ON") {
            digitalWrite(LIGHTS_PIN, HIGH);
            Serial.println("LED turned ON");
        } else if (rxValue == "OFF") {
            digitalWrite(LIGHTS_PIN, LOW);
            Serial.println("LED turned OFF");
        } else {
            Serial.println("Unknown command");
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LIGHTS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);

    Serial.println("Starting BLE...");

    // Init BLE
    BLEDevice::init("ESP32-Control");  // device name shown on your phone

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

    pCharacteristic->setCallbacks(new MyCallbacks());
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
    // Nothing needed here yet – all handled by callbacks.
    delay(1000);
}
