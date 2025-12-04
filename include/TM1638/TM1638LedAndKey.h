#pragma once

#include <Arduino.h>

class TM1638LedAndKeyModule {
public:
    const int strobe = 25; // STB to D8
    const int clk = 26; // CLK to D10
    const int data = 27; // DIO to D9

    void sendCommand(uint8_t value);

    void reset();

    uint8_t readButtons(void);

    void setLed(uint8_t value, uint8_t position);

    void buttons();

    void begin();

};