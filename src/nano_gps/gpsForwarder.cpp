#include "nano_gps/GpsForwarder.h"

GpsForwarder::GpsForwarder(const GpsForwarderConfig &config)
        : config_(config), espSerial_(config.espRxPin, config.espTxPin) {}

void GpsForwarder::begin() {
    Serial.begin(config_.serialBaud);
    espSerial_.begin(config_.espBaud);

    Serial.println(F("GPS on HW UART, ESP32 on SoftSerial"));
    espSerial_.println(F("Hello from nano."));
}

void GpsForwarder::update() {
    readGpsFromHardware();

    if (!shouldSend()) {
        return;
    }

    if (gps_.location.isValid()) {
        sendFix();
    } else {
        sendNoFix();
    }
}

void GpsForwarder::readGpsFromHardware() {
    while (Serial.available()) {
        gps_.encode(Serial.read());
    }
}

bool GpsForwarder::shouldSend() const {
    return millis() - lastSendMs_ >= config_.sendIntervalMs;
}

void GpsForwarder::sendNoFix() {
    lastSendMs_ = millis();
    Serial.println(F("NOFIX"));
    espSerial_.println(F("NOFIX"));
}

const char* GpsForwarder::messageTypeToString(MessageType t) {
    switch (t) {
        case MessageType::DATA:  return "DATA";
        case MessageType::ERROR: return "ERROR";
        case MessageType::PING:  return "PING";
    }
    return "UNKNOWN";
}

void GpsForwarder::sendMessage(Stream& out, const Message& msg) {
    // Simple text protocol: TYPE:payload\n
    out.print(messageTypeToString(msg.type));
    out.print(':');
    out.write(msg.payload, msg.length);
    out.println();
}

void GpsForwarder::sendFix() {
    lastSendMs_ = millis();

    const double lat  = gps_.location.lat();
    const double lon  = gps_.location.lng();
    const double spd  = gps_.speed.kmph();
    const double alt  = gps_.altitude.meters();
    const uint32_t sats = gps_.satellites.value();

    Message msg(MessageType::DATA);

    // Build the CSV payload into the message buffer
    msg.length = snprintf(
            msg.payload,
            Message::MaxPayloadSize,
            "GPS,%.6f,%.6f,%.2f,%.1f,%lu",
            lat,
            lon,
            spd,
            alt,
            static_cast<unsigned long>(sats)
    );

    if (msg.length >= Message::MaxPayloadSize) {
        // truncated – optional: handle error, cap length
        msg.length = Message::MaxPayloadSize - 1;
    }

    // Send to both outputs using the same standard format
    sendMessage(Serial, msg);
    sendMessage(espSerial_, msg);
}