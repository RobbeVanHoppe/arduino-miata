#include "nano_gps/gpsForwarder.h"

GpsForwarder::GpsForwarder(const GpsForwarderConfig &config)
        : config_(config), espSerial_(config.espRxPin, config.espTxPin) {}

void GpsForwarder::begin() {
    Serial.begin(config_.serialBaud);
    espSerial_.begin(config_.espBaud);

    Message startup(TYPE_INFO, NODE_GPS_ARDUINO, NODE_ESP32, "GPS on HW UART, ESP32 on SoftSerial");
    sendMessage(Serial, startup);
    sendMessage(espSerial_, startup);

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

void GpsForwarder::sendMessage(Stream& out, const Message& msg) {
    // Simple text protocol: TYPE:SRC>DST:payload\\n
    out.print(messageTypeToString(msg.type));
    out.print(':');
    out.print(messageNodeToString(msg.source));
    out.print('>');
    out.print(messageNodeToString(msg.destination));
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
    msg.source = MessageNode::NODE_GPS_ARDUINO;
    msg.destination = MessageNode::NODE_ESP32;

    char csvPayload[Message::MaxPayloadSize] = {0};
    const int written = snprintf(
            csvPayload,
            sizeof(csvPayload),
            "GPS,%.6f,%.6f,%.2f,%.1f,%lu",
            lat,
            lon,
            spd,
            alt,
            static_cast<unsigned long>(sats)
    );

    if (written > 0) {
        msg.setPayload(csvPayload);
    } else {
        msg.clearPayload();
    }

    // Send to both outputs using the same standard format
    sendMessage(Serial, msg);
    sendMessage(espSerial_, msg);
}