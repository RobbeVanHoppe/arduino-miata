#include <Arduino.h>

#include "nano_gps/GpsForwarder.h"

namespace {
    GpsForwarderConfig makeConfig() {
        GpsForwarderConfig config{};
        config.espTxPin = 6;          // Nano TX -> ESP32 RX (via divider)
        config.espRxPin = 7;          // Nano RX <- ESP32 TX
        config.espBaud = 115200;
        config.serialBaud = 9600;     // Hardware UART shared with GPS + USB
        config.sendIntervalMs = 1000; // send once per second
        return config;
    }

    GpsForwarder gpsForwarder(makeConfig());
}

void setup() {
    gpsForwarder.begin();
}

void loop() {
    gpsForwarder.update();
}