#include "esp32_dash/GPS/gpsHandler.h"

#include <cstdio>
#include <cstring>

#include "common/message.h"

namespace {
    bool startsWith(const char *text, const char *prefix) {
        return text && prefix && strncmp(text, prefix, strlen(prefix)) == 0;
    }
}

void gpsHandler::begin(Stream &serial) {
    serial_ = &serial;
    bufferLength_ = 0;
    lastLine_[0] = '\0';
    hasFix_ = false;
    noFix_ = false;
    lastFix_ = {};
    lastByteMs_ = 0;
    bytesReceived_ = 0;
}

bool gpsHandler::update() {
    if (!serial_) {
        return false;
    }

    bool parsedFrame = false;

    while (serial_->available()) {
        const char incoming = static_cast<char>(serial_->read());
        lastByteMs_ = millis();
        ++bytesReceived_;

        if (incoming == '\r') {
            continue;
        }

        if (incoming == '\n') {
            buffer_[bufferLength_] = '\0';
            strncpy(lastLine_, buffer_, sizeof(lastLine_) - 1);
            lastLine_[sizeof(lastLine_) - 1] = '\0';

            const ParseResult result = processLine(buffer_);
            parsedFrame = parsedFrame || (result != ParseResult::Ignored);

            bufferLength_ = 0;
            continue;
        }

        if (bufferLength_ < sizeof(buffer_) - 1) {
            buffer_[bufferLength_++] = incoming;
        } else {
            // overflow: drop partial line and wait for next newline
            bufferLength_ = 0;
        }
    }

    return parsedFrame;
}

gpsHandler::ParseResult gpsHandler::processLine(const char *line) {
    if (!line || line[0] == '\0') {
        return ParseResult::Ignored;
    }

    if (strcmp(line, "NOFIX") == 0) {
        noFix_ = true;
        hasFix_ = false;
        lastFix_.valid = false;
        return ParseResult::NoFix;
    }

    return parseMessageFrame(line);
}

gpsHandler::ParseResult gpsHandler::parseMessageFrame(const char *line) {
    // protocol: TYPE:SRC>DST:payload
    const char *firstColon = strchr(line, ':');
    if (!firstColon) {
        return ParseResult::Ignored;
    }

    const size_t typeLength = static_cast<size_t>(firstColon - line);
    if (typeLength == 0 || typeLength >= 8) {
        return ParseResult::Ignored;
    }

    char typeBuffer[8] = {0};
    memcpy(typeBuffer, line, typeLength);

    if (strcmp(typeBuffer, messageTypeToString(MessageType::TYPE_DATA)) != 0) {
        return ParseResult::Ignored;
    }

    const char *routingStart = firstColon + 1;
    const char *routeSeparator = strchr(routingStart, '>');
    if (!routeSeparator) {
        return ParseResult::Ignored;
    }

    const char *secondColon = strchr(routeSeparator, ':');
    if (!secondColon) {
        return ParseResult::Ignored;
    }

    const size_t srcLength = static_cast<size_t>(routeSeparator - routingStart);
    const size_t dstLength = static_cast<size_t>(secondColon - routeSeparator - 1);
    if (srcLength == 0 || srcLength >= 6 || dstLength == 0 || dstLength >= 6) {
        return ParseResult::Ignored;
    }

    char srcNodeRaw[6] = {0};
    char dstNodeRaw[6] = {0};
    memcpy(srcNodeRaw, routingStart, srcLength);
    memcpy(dstNodeRaw, routeSeparator + 1, dstLength);

    const MessageNode source = parseMessageNode(srcNodeRaw);
    const MessageNode destination = parseMessageNode(dstNodeRaw);

    if (source != MessageNode::NODE_GPS_ARDUINO) {
        return ParseResult::Ignored;
    }

    if (destination != MessageNode::NODE_ESP32 && destination != MessageNode::NODE_UNKNOWN) {
        return ParseResult::Ignored;
    }

    const char *payload = secondColon + 1;
    return parseGpsPayload(payload);
}

gpsHandler::ParseResult gpsHandler::parseGpsPayload(const char *payload) {
    // payload from sender: GPS,lat,lon,speed_kmph,alt_m,sats
    if (!startsWith(payload, "GPS,")) {
        return ParseResult::Ignored;
    }

    double latitude = 0.0;
    double longitude = 0.0;
    float speedKmph = 0.0f;
    float altitudeMeters = 0.0f;
    unsigned long satellites = 0;

    if (sscanf(payload + 4, "%lf,%lf,%f,%f,%lu", &latitude, &longitude, &speedKmph, &altitudeMeters, &satellites) != 5) {
        return ParseResult::Ignored;
    }

    lastFix_.latitude = latitude;
    lastFix_.longitude = longitude;
    lastFix_.speedKmph = speedKmph;
    lastFix_.altitudeMeters = altitudeMeters;
    lastFix_.satellites = static_cast<uint32_t>(satellites);
    lastFix_.lastUpdateMs = millis();
    lastFix_.valid = true;

    hasFix_ = true;
    noFix_ = false;

    return ParseResult::Fix;
}

void gpsHandler::sendCommand(const char *cmd) {
    if (!serial_ || !cmd || cmd[0] == '\0') {
        return;
    }

    serial_->println(cmd);
}

uint32_t gpsHandler::lastByteMs() const {
    return lastByteMs_;
}

uint32_t gpsHandler::bytesReceived() const {
    return bytesReceived_;
}

const char *gpsHandler::readBuffer() const {
    return lastLine_;
}

bool gpsHandler::hasFix() const {
    return hasFix_;
}

const GpsFix &gpsHandler::fix() const {
    return lastFix_;
}

bool gpsHandler::lastMessageWasNoFix() const {
    return noFix_;
}
