#pragma once

#include <Arduino.h>

class TM1638LedAndKeyModule {
public:
    TM1638LedAndKeyModule(uint8_t strobePin, uint8_t clkPin, uint8_t dataPin);

    void begin();

    uint8_t readButtons();               // Returns 8-bit mask of all buttons
    void setLed(uint8_t value, uint8_t position);  // value: 0 or 1, position: 0–7
    void reset();

private:
    void sendCommand(uint8_t value);

    uint8_t strobe;
    uint8_t clk;
    uint8_t data;
};
