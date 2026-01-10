#pragma once

#include <Arduino.h>

struct GpsFix {
    double latitude = 0.0;
    double longitude = 0.0;
    float speedKmph = 0.0f;
    float altitudeMeters = 0.0f;
    uint32_t satellites = 0;
    bool valid = false;
    uint32_t lastUpdateMs = 0;
};

class gpsHandler {
public:
    void begin(Stream &serial);
    bool update();
    void sendCommand(char *cmd);
    const char *readBuffer() const;
    bool hasFix() const;
    const GpsFix &fix() const;
    bool lastMessageWasNoFix() const;

private:
    void processLine(const char *line);

    Stream *serial_ = nullptr;
    char buffer_[128] = {0};
    size_t bufferLength_ = 0;
    char lastLine_[128] = {0};
    bool hasFix_ = false;
    bool noFix_ = false;
    GpsFix lastFix_;
};
