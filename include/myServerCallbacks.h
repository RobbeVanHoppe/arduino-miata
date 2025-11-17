//
// Created by robbe on 17/11/2025.
//
#pragma once

#include "main.h"

class MyServerCallbacks : public BLEServerCallbacks {
    bool deviceConnected = false;

    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        Serial.println("Client disconnected");
        // Make advertising restart so you can reconnect
        pServer->getAdvertising()->start();
    }
};