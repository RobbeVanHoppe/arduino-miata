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
        _waterPage.setStatusMessage("Client connected");
        _displayManager.requestRefresh();
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer *server) override {
        _waterPage.setStatusMessage("Awaiting client");
        _displayManager.requestRefresh();
        Serial.println("Client disconnected");
        server->getAdvertising()->start();
    }

private:
    WaterTempPage &_waterPage;
    DisplayManager &_displayManager;
};
