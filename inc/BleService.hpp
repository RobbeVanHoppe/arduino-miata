#pragma once

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <cmath>
#include <cstdio>

#include <string>

#include "Config.hpp"
#include "SensorReadings.hpp"

/**
 * Transport-agnostic command/telemetry bridge that currently uses the
 * built-in ESP32 Bluetooth Serial (SPP) stack so no third-party BLE
 * libraries are required.
 */
template <typename CommandHandler>
class BleService {
public:
    explicit BleService(CommandHandler& handler)
        : handler_(handler) {}

    void begin();
    void loop(const SensorReadings& readings);

private:
    void processIncomingCommands();
    void sendTelemetry(const SensorReadings& readings);

    CommandHandler& handler_;
    BluetoothSerial btSerial_;
    bool deviceConnected_ = false;
    unsigned long lastNotifyMs_ = 0;
};

template <typename CommandHandler>
void BleService<CommandHandler>::begin() {
    Serial.println("Starting Bluetooth Serial...");
    if (!btSerial_.begin(Config::BLUETOOTH_DEVICE_NAME)) {
        Serial.println("Failed to start Bluetooth stack");
        return;
    }
    Serial.print("Bluetooth device ready as ");
    Serial.println(Config::BLUETOOTH_DEVICE_NAME);
}

template <typename CommandHandler>
void BleService<CommandHandler>::loop(const SensorReadings& readings) {
    bool hasClient = btSerial_.hasClient();
    if (hasClient && !deviceConnected_) {
        Serial.println("Bluetooth client connected");
        deviceConnected_ = true;
    } else if (!hasClient && deviceConnected_) {
        Serial.println("Bluetooth client disconnected");
        deviceConnected_ = false;
    }

    if (!hasClient) {
        return;
    }

    processIncomingCommands();

    unsigned long now = millis();
    if (now - lastNotifyMs_ >= Config::BLUETOOTH_NOTIFY_INTERVAL_MS) {
        sendTelemetry(readings);
        lastNotifyMs_ = now;
    }
}

template <typename CommandHandler>
void BleService<CommandHandler>::processIncomingCommands() {
    while (btSerial_.available()) {
        String line = btSerial_.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) {
            continue;
        }
        std::string command(line.c_str(), line.length());
        Serial.print("Received command: ");
        Serial.println(line);
        handler_.handleCommand(command);
    }
}

template <typename CommandHandler>
void BleService<CommandHandler>::sendTelemetry(const SensorReadings& readings) {
    const float waterTemp = std::isnan(readings.waterTempC) ? -1.0f : readings.waterTempC;
    const float oilPressure = std::isnan(readings.oilPressurePsi) ? -1.0f : readings.oilPressurePsi;

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"rpm\":%u,\"water_c\":%.2f,\"oil_psi\":%.2f,\"handbrake\":%s}",
             readings.rpm,
             waterTemp,
             oilPressure,
             readings.handbrakeEngaged ? "true" : "false");
    btSerial_.println(payload);
}

