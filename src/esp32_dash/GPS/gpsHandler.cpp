#include "esp32_dash/GPS/gpsHandler.h"

#include <cstdio>
#include <cstring>

#include "common/message.h"

void gpsHandler::begin(Stream &serial) {
    serial_ = &serial;
    bufferLength_ = 0;
    lastLine_[0] = '\0';
    hasFix_ = false;
    noFix_ = false;
    lastByteMs_ = 0;
    bytesReceived_ = 0;
}

bool gpsHandler::update() {
    if (!serial_) {
        return false;
    }

    bool processed = false;
    while (serial_->available()) {
        const char c = static_cast<char>(serial_->read());
        lastByteMs_ = millis();
        ++bytesReceived_;
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            buffer_[bufferLength_] = '\0';
            strncpy(lastLine_, buffer_, sizeof(lastLine_) - 1);
            lastLine_[sizeof(lastLine_) - 1] = '\0';
            processLine(buffer_);
            bufferLength_ = 0;
            processed = true;
            continue;
        }

        if (bufferLength_ < sizeof(buffer_) - 1) {
            buffer_[bufferLength_++] = c;
        } else {
            bufferLength_ = 0;
        }
    }

    return processed;
}

void gpsHandler::processLine(const char *line) {
    if (!line || line[0] == '\0') {
        return;
    }

    if (strcmp(line, "NOFIX") == 0) {
        noFix_ = true;
        hasFix_ = false;
        return;
    }

    if (strncmp(line, "DATA:", 5) != 0) {
        return;
    }

    const char *payload = line + 5;
    const char *csv = payload;

    const char *firstColon = strchr(payload, ':');
    const char *routeSep = strchr(payload, '>');
    if (firstColon && routeSep && routeSep < firstColon) {
        const size_t srcLen = static_cast<size_t>(routeSep - payload);
        const size_t dstLen = static_cast<size_t>(firstColon - routeSep - 1);

        char srcNode[6] = {0};
        char dstNode[6] = {0};

        if (srcLen > 0 && srcLen < sizeof(srcNode) && dstLen > 0 && dstLen < sizeof(dstNode)) {
            memcpy(srcNode, payload, srcLen);
            memcpy(dstNode, routeSep + 1, dstLen);

            const MessageNode source = parseMessageNode(srcNode);
            const MessageNode destination = parseMessageNode(dstNode);
            if (source != MessageNode::NODE_GPS_ARDUINO) {
                return;
            }
            if (destination != MessageNode::NODE_ESP32 && destination != MessageNode::NODE_UNKNOWN) {
                return;
            }

            csv = firstColon + 1;
        }
    }

    if (strncmp(csv, "GPS,", 4) != 0) {
        return;
    }

    csv += 4;
    double lat = 0.0;
    double lon = 0.0;
    float spd = 0.0f;
    float alt = 0.0f;
    unsigned long sats = 0;

    if (sscanf(csv, "%lf,%lf,%f,%f,%lu", &lat, &lon, &spd, &alt, &sats) == 5) {
        lastFix_.latitude = lat;
        lastFix_.longitude = lon;
        lastFix_.speedKmph = spd;
        lastFix_.altitudeMeters = alt;
        lastFix_.satellites = static_cast<uint32_t>(sats);
        lastFix_.valid = true;
        lastFix_.lastUpdateMs = millis();
        hasFix_ = true;
        noFix_ = false;
    }
}

void gpsHandler::sendCommand(char *cmd) {}

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
