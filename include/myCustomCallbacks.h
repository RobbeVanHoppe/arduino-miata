#pragma once

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/WaterTempPage.h"

class MyCustomCallbacks : public BLECharacteristicCallbacks {
public:
    MyCustomCallbacks(WaterTempPage &waterPage, DisplayManager &displayManager)
            : _waterPage(waterPage), _displayManager(displayManager) {}

    void onWrite(BLECharacteristic *characteristic) override {
        std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        if (rxValue == "ON") {
            digitalWrite(LIGHTS_PIN, HIGH);
            _waterPage.setStatusMessage("Lights ON");
            Serial.println("LED turned ON");
        } else if (rxValue == "OFF") {
            digitalWrite(LIGHTS_PIN, LOW);
            _waterPage.setStatusMessage("Lights OFF");
            Serial.println("LED turned OFF");
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
