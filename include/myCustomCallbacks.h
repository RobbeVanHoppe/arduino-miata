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

        if (rxValue == "Lights") {
            if (_lightsAreOff) {
                digitalWrite(LIGHTS_PIN, HIGH);
                showTransientStatusMessage(F("Lights ON"));
                _lightsAreOff = false;
            } else {
                digitalWrite(LIGHTS_PIN, LOW);
                showTransientStatusMessage(F("Lights OFF"));
                _lightsAreOff = true;
            }
        } else if (rxValue == "Start") {
            // TODO: Write a sequence to start the engine, looking at the RPMS being 0 and the handbrake being enabled

        } else {
            showTransientStatusMessage(F("Unknown cmd"));
            Serial.println("Unknown command");
        }
    }

private:
    // TODO: get a digitalRead from the light switch
    bool _lightsAreOff = false;
};
