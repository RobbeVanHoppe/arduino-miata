#include "TM1638/TM1638LedAndKey.h"


void TM1638LedAndKeyModule::begin() {
    pinMode(strobe, OUTPUT);
    pinMode(clk, OUTPUT);
    pinMode(data, OUTPUT);
    sendCommand(0x8f);  // Activate with maximum display brightness
    reset();

}

void TM1638LedAndKeyModule::sendCommand(uint8_t value) {
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, value);
    digitalWrite(strobe, HIGH);
}

void TM1638LedAndKeyModule::reset() {
    sendCommand(0x40); // Set auto increment mode
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0xc0);   // Set starting address to 0
    for (uint8_t i = 0; i < 16; i++) {
        shiftOut(data, clk, LSBFIRST, 0x00);
    }
    digitalWrite(strobe, HIGH);
}

uint8_t TM1638LedAndKeyModule::readButtons(void) {
    uint8_t buttons = 0;
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0x42);
    pinMode(data, INPUT);
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t v = shiftIn(data, clk, LSBFIRST) << i;
        buttons |= v;
    }
    pinMode(data, OUTPUT);
    digitalWrite(strobe, HIGH);
    return buttons;
}

void TM1638LedAndKeyModule::setLed(uint8_t value, uint8_t position) {
    pinMode(data, OUTPUT);
    sendCommand(0x44);
    digitalWrite(strobe, LOW);
    shiftOut(data, clk, LSBFIRST, 0xC1 + (position << 1));
    shiftOut(data, clk, LSBFIRST, value);
    digitalWrite(strobe, HIGH);
}

void TM1638LedAndKeyModule::buttons() {

}
