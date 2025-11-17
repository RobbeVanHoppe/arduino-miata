#pragma once

#include <Arduino.h>

#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

extern BLEServer *pServer;
extern BLECharacteristic *pCharacteristic;

const int LIGHTS_PIN = 2;
const int WINDOWS_PIN = 3;
