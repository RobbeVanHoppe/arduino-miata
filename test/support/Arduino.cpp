#include "Arduino.h"

#include <cstddef>
#include <vector>

namespace {
unsigned long currentMillis = 0;
unsigned long currentMicros = 0;
std::vector<int> analogValues;
size_t analogIndex = 0;
}

HardwareSerial Serial(0);

unsigned long millis() {
    return currentMillis;
}

unsigned long micros() {
    return currentMicros;
}

void setMillis(unsigned long value) {
    currentMillis = value;
}

void advanceMillis(unsigned long delta) {
    currentMillis += delta;
    currentMicros += delta * 1000;
}

void advanceMicros(unsigned long delta) {
    currentMicros += delta;
    currentMillis = currentMicros / 1000;
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

void attachInterrupt(int, void (*isr)(void), int mode) {
    // In tests we call the ISR manually when needed.
    (void)isr;
    (void)mode;
}
