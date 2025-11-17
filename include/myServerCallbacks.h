#pragma once

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/WaterTempPage.h"

class MyServerCallbacks : public BLEServerCallbacks {
public:
    MyServerCallbacks(WaterTempPage &waterPage, DisplayManager &displayManager)
            : _waterPage(waterPage), _displayManager(displayManager) {}

    void onConnect(BLEServer *server) override {
        (void) server;
        showTransientStatusMessage("Phone connected");
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer *server) override {
        showTransientStatusMessage("Phone disconnected");
        Serial.println("Client disconnected");
        server->getAdvertising()->start();
    }

private:
    WaterTempPage &_waterPage;
    DisplayManager &_displayManager;
};
