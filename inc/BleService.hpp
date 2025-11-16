#pragma once

#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

extern "C" {
#include <freertos/ringbuf.h>
}

#include <cstdio>
#include <string>

#include "Config.hpp"
#include "SensorReadings.hpp"

template <typename CommandHandler>
class BleService {
public:
    explicit BleService(CommandHandler& handler)
        : handler_(handler) {}

    void begin();
    void loop(const SensorReadings& readings);

private:
    class ServerCallbacks : public BLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleService& service) : service_(service) {}
        void onConnect(BLEServer* server) override {
            service_.deviceConnected_ = true;
            Serial.println("Client connected");
        }
        void onDisconnect(BLEServer* server) override {
            service_.deviceConnected_ = false;
            Serial.println("Client disconnected");
            server->getAdvertising()->start();
        }
    private:
        BleService& service_;
    };

    class CommandCallbacks : public BLECharacteristicCallbacks {
    public:
        explicit CommandCallbacks(BleService& service) : service_(service) {}
        void onWrite(BLECharacteristic* characteristic) override {
            std::string rxValue = characteristic->getValue();
            if (rxValue.empty()) {
                return;
            }
            Serial.print("Received: ");
            Serial.println(rxValue.c_str());
            service_.handler_.handleCommand(rxValue);
        }
    private:
        BleService& service_;
    };

    void updateTelemetryCharacteristic(const SensorReadings& readings);

    CommandHandler& handler_;
    BLEServer* server_ = nullptr;
    BLECharacteristic* commandCharacteristic_ = nullptr;
    BLECharacteristic* telemetryCharacteristic_ = nullptr;
    bool deviceConnected_ = false;
    unsigned long lastNotifyMs_ = 0;
};

template <typename CommandHandler>
void BleService<CommandHandler>::begin() {
    Serial.println("Starting BLE...");
    BLEDevice::init("ESP32-Control");

    server_ = BLEDevice::createServer();
    server_->setCallbacks(new ServerCallbacks(*this));

    BLEService* service = server_->createService(Config::SERVICE_UUID);

    commandCharacteristic_ = service->createCharacteristic(
        Config::COMMAND_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY);
    commandCharacteristic_->setCallbacks(new CommandCallbacks(*this));
    commandCharacteristic_->setValue("READY");

    telemetryCharacteristic_ = service->createCharacteristic(
        Config::TELEMETRY_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY);
    telemetryCharacteristic_->addDescriptor(new BLE2902());
    telemetryCharacteristic_->setValue("{}");

    service->start();

    BLEAdvertising* advertising = server_->getAdvertising();
    advertising->addServiceUUID(Config::SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE device ready, advertising telemetry service");
}

template <typename CommandHandler>
void BleService<CommandHandler>::loop(const SensorReadings& readings) {
    unsigned long now = millis();
    if (deviceConnected_ && now - lastNotifyMs_ >= Config::BLE_NOTIFY_INTERVAL_MS) {
        updateTelemetryCharacteristic(readings);
        lastNotifyMs_ = now;
    }
}

template <typename CommandHandler>
void BleService<CommandHandler>::updateTelemetryCharacteristic(const SensorReadings& readings) {
    if (!telemetryCharacteristic_) {
        return;
    }
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"rpm\":%u,\"water_c\":%.2f,\"oil_psi\":%.2f,\"handbrake\":%s}",
             readings.rpm,
             isnan(readings.waterTempC) ? -1.0f : readings.waterTempC,
             isnan(readings.oilPressurePsi) ? -1.0f : readings.oilPressurePsi,
             readings.handbrakeEngaged ? "true" : "false");

    telemetryCharacteristic_->setValue(payload);
    telemetryCharacteristic_->notify();
}
