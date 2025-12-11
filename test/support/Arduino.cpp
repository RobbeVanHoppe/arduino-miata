#include "Arduino.h"

#include <cstddef>
#include <vector>

namespace {
unsigned long currentMillis = 0;
std::vector<int> analogValues;
size_t analogIndex = 0;
}

HardwareSerial Serial(0);

unsigned long millis() {
    return currentMillis;
}

void setMillis(unsigned long value) {
    currentMillis = value;
}

void advanceMillis(unsigned long delta) {
    currentMillis += delta;
}

int analogRead(uint8_t) {
    if (analogValues.empty()) {
        return 0;
    }
    int value = analogValues[analogIndex % analogValues.size()];
    analogIndex++;
    return value;
}

void setAnalogReadSequence(const std::vector<int> &values) {
    analogValues = values;
    analogIndex = 0;
}
