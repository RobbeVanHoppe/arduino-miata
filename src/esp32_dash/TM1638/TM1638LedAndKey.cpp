#include "esp32_dash/TM1638/TM1638LedAndKey.h"

TM1638LedAndKeyModule::TM1638LedAndKeyModule(uint8_t strobePin,
                                             uint8_t clkPin,
                                             uint8_t dataPin)
        : strobe(strobePin), clk(clkPin), data(dataPin) {}

void TM1638LedAndKeyModule::begin() {
    pinMode(strobe, OUTPUT);
    pinMode(clk, OUTPUT);
    pinMode(data, OUTPUT);

    digitalWrite(strobe, HIGH);
    digitalWrite(clk, HIGH);
    digitalWrite(data, HIGH);

    sendCommand(0x8F);   // Display ON, max brightness
    reset();
}

void TM1638LedAndKeyModule::sendCommand(uint8_t value) {
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, value);
    digitalWrite(strobe, HIGH);
}

void TM1638LedAndKeyModule::reset() {
    sendCommand(0x40);   // Auto-increment mode
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0xC0);  // Address = 0

    for (uint8_t i = 0; i < 16; i++) {
        shiftOut(data, clk, LSBFIRST, 0x00);
    }

    digitalWrite(strobe, HIGH);
}

uint8_t TM1638LedAndKeyModule::readButtons() {
    uint8_t buttons = 0;

    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0x42);  // Read buttons command
    pinMode(data, INPUT);

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t v = shiftIn(data, clk, LSBFIRST);
        buttons |= (v << i);
    }

    pinMode(data, OUTPUT);
    digitalWrite(strobe, HIGH);

    return buttons;
}

void TM1638LedAndKeyModule::setLed(uint8_t value, uint8_t position) {
    if (position > 7) return;

    sendCommand(0x44);        // Fixed address mode
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0xC1 + (position << 1));
    shiftOut(data, clk, LSBFIRST, value ? 0xFF : 0x00);
    digitalWrite(strobe, HIGH);
}
