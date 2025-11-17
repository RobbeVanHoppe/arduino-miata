#pragma once

#include "main.h"
#include "display/DisplayManager.h"
#include "display/pages/StaticTextPage.h"

class MyServerCallbacks : public BLEServerCallbacks {
public:
    MyServerCallbacks(StaticTextPage &statusPage, DisplayManager &displayManager)
            : _statusPage(statusPage), _displayManager(displayManager) {}

    void onConnect(BLEServer *server) override {
        (void) server;
        _statusPage.setBody("Client connected");
        _displayManager.requestRefresh();
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer *server) override {
        _statusPage.setBody("Awaiting client");
        _displayManager.requestRefresh();
        Serial.println("Client disconnected");
        server->getAdvertising()->start();
    }

private:
    StaticTextPage &_statusPage;
    DisplayManager &_displayManager;
};
