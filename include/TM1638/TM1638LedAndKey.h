#pragma once

#include <Arduino.h>

class TM1638LedAndKeyModule {
public:
    struct Config {
        uint8_t dioPin = 32;
        uint8_t clkPin = 33;
        uint8_t stbPin = 34;
        uint8_t brightness = 5;  // range: 0 (dim) - 7 (bright)
        bool displayEnabled = true;
    };

    explicit TM1638LedAndKeyModule(const Config &config);

    void begin();
    void setBrightness(uint8_t brightness);
    void setDisplayEnabled(bool enabled);

    void clearDisplay();
    void setSegments(const uint8_t segments[8]);
    void setDigits(const int8_t digits[8], uint8_t dotsMask = 0);
    void setDigit(uint8_t position, int8_t digit, bool dot = false);
    void setLeds(uint8_t ledMask);

    // Returns an 8-bit mask indicating which keys are pressed (bit 0 = K1, bit 7 = K8).
    uint8_t readKeys();

private:
    void writeDisplay();
    void sendCommand(uint8_t command);
    void sendData(uint8_t address, uint8_t data);
    void startTransaction();
    void endTransaction();
    void shiftOutByte(uint8_t data);
    uint8_t shiftInByte();
    void applyDisplayControl();

    Config config_;
    uint8_t displayBuffer_[8] = {0};
    uint8_t ledMask_ = 0;
    static const uint8_t kDigitSegments[16];
};

