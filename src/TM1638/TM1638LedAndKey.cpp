#include "TM1638/TM1638LedAndKey.h"

namespace {
constexpr uint8_t kAutoIncrementCommand = 0x40;
constexpr uint8_t kFixedAddressCommand = 0x44;
constexpr uint8_t kReadKeyCommand = 0x42;
constexpr uint8_t kDisplayAddress = 0xC0;
}  // namespace

const uint8_t TM1638LedAndKeyModule::kDigitSegments[16] = {
        0b00111111,  // 0
        0b00000110,  // 1
        0b01011011,  // 2
        0b01001111,  // 3
        0b01100110,  // 4
        0b01101101,  // 5
        0b01111101,  // 6
        0b00000111,  // 7
        0b01111111,  // 8
        0b01101111,  // 9
        0b01110111,  // A
        0b01111100,  // b
        0b00111001,  // C
        0b01011110,  // d
        0b01111001,  // E
        0b01110001,  // F
};

TM1638LedAndKeyModule::TM1638LedAndKeyModule(const Config &config)
        : config_(config) {}

void TM1638LedAndKeyModule::begin() {
    pinMode(config_.dioPin, OUTPUT);
    pinMode(config_.clkPin, OUTPUT);
    pinMode(config_.stbPin, OUTPUT);

    digitalWrite(config_.stbPin, HIGH);
    digitalWrite(config_.clkPin, HIGH);

    applyDisplayControl();
    clearDisplay();
}

void TM1638LedAndKeyModule::setBrightness(uint8_t brightness) {
    config_.brightness = constrain(brightness, static_cast<uint8_t>(0), static_cast<uint8_t>(7));
    applyDisplayControl();
}

void TM1638LedAndKeyModule::setDisplayEnabled(bool enabled) {
    config_.displayEnabled = enabled;
    applyDisplayControl();
}

void TM1638LedAndKeyModule::clearDisplay() {
    for (auto &value: displayBuffer_) {
        value = 0;
    }
    ledMask_ = 0;
    writeDisplay();
}

void TM1638LedAndKeyModule::setSegments(const uint8_t segments[8]) {
    for (uint8_t i = 0; i < 8; ++i) {
        displayBuffer_[i] = segments[i];
    }
    writeDisplay();
}

void TM1638LedAndKeyModule::setDigits(const int8_t digits[8], uint8_t dotsMask) {
    uint8_t segments[8]{};
    for (uint8_t i = 0; i < 8; ++i) {
        if (digits[i] >= 0 && digits[i] < 16) {
            segments[i] = kDigitSegments[digits[i]];
        }
        if (dotsMask & (1 << i)) {
            segments[i] |= 0x80;
        }
    }
    setSegments(segments);
}

void TM1638LedAndKeyModule::setDigit(uint8_t position, int8_t digit, bool dot) {
    if (position >= 8) {
        return;
    }
    uint8_t segment = 0;
    if (digit >= 0 && digit < 16) {
        segment = kDigitSegments[digit];
    }
    if (dot) {
        segment |= 0x80;
    }
    displayBuffer_[position] = segment;
    writeDisplay();
}

void TM1638LedAndKeyModule::setLeds(uint8_t ledMask) {
    ledMask_ = ledMask;
    writeDisplay();
}

uint8_t TM1638LedAndKeyModule::readKeys() {
    uint8_t keys = 0;
    startTransaction();
    shiftOutByte(kReadKeyCommand);

    pinMode(config_.dioPin, INPUT);
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t data = shiftInByte();
        keys |= (data & 0x11) << i;
    }
    pinMode(config_.dioPin, OUTPUT);
    endTransaction();

    return keys;
}

void TM1638LedAndKeyModule::writeDisplay() {
    sendCommand(kAutoIncrementCommand);

    startTransaction();
    shiftOutByte(kDisplayAddress);
    for (uint8_t position = 0; position < 8; ++position) {
        shiftOutByte(displayBuffer_[position]);
        shiftOutByte((ledMask_ & (1 << position)) ? 0x01 : 0x00);
    }
    endTransaction();
}

void TM1638LedAndKeyModule::sendCommand(uint8_t command) {
    startTransaction();
    shiftOutByte(command);
    endTransaction();
}

void TM1638LedAndKeyModule::sendData(uint8_t address, uint8_t data) {
    sendCommand(kFixedAddressCommand);
    startTransaction();
    shiftOutByte(kDisplayAddress | address);
    shiftOutByte(data);
    endTransaction();
}

void TM1638LedAndKeyModule::startTransaction() {
    digitalWrite(config_.stbPin, LOW);
}

void TM1638LedAndKeyModule::endTransaction() {
    digitalWrite(config_.stbPin, HIGH);
}

void TM1638LedAndKeyModule::shiftOutByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(config_.clkPin, LOW);
        digitalWrite(config_.dioPin, data & 0x01);
        data >>= 1;
        digitalWrite(config_.clkPin, HIGH);
    }
}

uint8_t TM1638LedAndKeyModule::shiftInByte() {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(config_.clkPin, LOW);
        if (digitalRead(config_.dioPin)) {
            data |= (1 << i);
        }
        digitalWrite(config_.clkPin, HIGH);
    }
    return data;
}

void TM1638LedAndKeyModule::applyDisplayControl() {
    const uint8_t brightness = constrain(config_.brightness, static_cast<uint8_t>(0), static_cast<uint8_t>(7));
    const uint8_t command = 0x80 | (config_.displayEnabled ? 0x08 : 0x00) | brightness;
    sendCommand(command);
}

