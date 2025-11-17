#pragma once

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/WaterTempPage.h"

class MyCustomCallbacks : public BLECharacteristicCallbacks {
public:
    MyCustomCallbacks(WaterTempPage &waterPage, DisplayManager &displayManager)
            : _waterPage(waterPage), _displayManager(displayManager) {}

    void onWrite(BLECharacteristic *characteristic) override {
        bool lightsAreOff = false;

        std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        if (rxValue == "LIGHTS") {
            if (lightsAreOff) {
                digitalWrite(LIGHTS_PIN, HIGH);
                _waterPage.setStatusMessage("Lights ON");
                lightsAreOff = false;

            } else {
                digitalWrite(LIGHTS_PIN, LOW);
                _waterPage.setStatusMessage("Lights OFF");
                lightsAreOff = true;
            }
        } else if (rxValue == "OFF") {

        } else {
            _waterPage.setStatusMessage("Unknown cmd");
            Serial.println("Unknown command");
        }
        _displayManager.requestRefresh();
    }

private:
    WaterTempPage &_waterPage;
    DisplayManager &_displayManager;
};
