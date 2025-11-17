//
// Created by robbe on 17/11/2025.
//
#pragma once

#include "main.h"

class MyCustomCallbacks : public BLECharacteristicCallbacks {
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