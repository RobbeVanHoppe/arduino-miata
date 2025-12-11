#pragma once

#include "main.h"
#include "esp32_dash/display/DisplayManager.h"
#include "esp32_dash/display/pages/WaterTempPage.h"

class MyServerCallbacks : public BLEServerCallbacks {
public:
    MyServerCallbacks(WaterTempPage &waterPage, DisplayManager &displayManager)
            : _waterPage(waterPage), _displayManager(displayManager) {}

    void onConnect(BLEServer *server) override {
        (void) server;
        showTransientStatusMessage("Connected");
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer *server) override {
        showTransientStatusMessage("Disconnected");
        Serial.println("Client disconnected");
        server->getAdvertising()->start();
    }

private:
    WaterTempPage &_waterPage;
    DisplayManager &_displayManager;
};
