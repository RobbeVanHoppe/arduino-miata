#pragma once

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"

class MyCustomCallbacks : public BLECharacteristicCallbacks {
public:
    MyCustomCallbacks(StaticTextPage &statusPage, DisplayManager &displayManager)
            : _statusPage(statusPage), _displayManager(displayManager) {}

    void onWrite(BLECharacteristic *characteristic) override {
        std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        if (rxValue == "ON") {
            digitalWrite(LIGHTS_PIN, HIGH);
            _statusPage.setBody("Lights ON");
            Serial.println("LED turned ON");
        } else if (rxValue == "OFF") {
            digitalWrite(LIGHTS_PIN, LOW);
            _statusPage.setBody("Lights OFF");
            Serial.println("LED turned OFF");
        } else {
            _statusPage.setBody("Unknown cmd");
            Serial.println("Unknown command");
        }
        _displayManager.requestRefresh();
    }

private:
    StaticTextPage &_statusPage;
    DisplayManager &_displayManager;
};
