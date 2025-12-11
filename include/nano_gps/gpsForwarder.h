#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include "common/message.h"

struct GpsForwarderConfig {
    int espTxPin;           // Nano TX -> ESP32 RX (via divider)
    int espRxPin;           // Nano RX <- ESP32 TX
    uint32_t espBaud;       // baud rate for SoftwareSerial link
    uint32_t serialBaud;    // baud rate for USB serial monitor and GPS module
    uint32_t sendIntervalMs;  // how often to forward a fix
};

class GpsForwarder {
public:
    explicit GpsForwarder(const GpsForwarderConfig &config);

    void begin();
    void update();

private:
    const char* messageTypeToString(MessageType t);
    void sendMessage(Stream& out, const Message& msg);
    void readGpsFromHardware();
    void sendFix();
    void sendNoFix();
    bool shouldSend() const;

    const GpsForwarderConfig config_;
    SoftwareSerial espSerial_;
    TinyGPSPlus gps_;
    unsigned long lastSendMs_ = 0;
};