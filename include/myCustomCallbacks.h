#pragma once

#include "main.h"

class MyCustomCallbacks : public BLECharacteristicCallbacks {
public:
    MyCustomCallbacks() = default;

    void onWrite(BLECharacteristic *characteristic) override {
        std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        if (rxValue == "LIGHTS") {
            if (_lightsAreOff) {
                digitalWrite(LIGHTS_PIN, HIGH);
                showTransientStatusMessage(F("Lights ON"));
                _lightsAreOff = false;
            } else {
                digitalWrite(LIGHTS_PIN, LOW);
                showTransientStatusMessage(F("Lights OFF"));
                _lightsAreOff = true;
            }
        } else if (rxValue == "OFF") {

        } else {
            showTransientStatusMessage(F("Unknown cmd"));
            Serial.println("Unknown command");
        }
    }

private:
    bool _lightsAreOff = true;
};
