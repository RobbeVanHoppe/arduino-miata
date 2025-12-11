#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

using byte = uint8_t;
using __FlashStringHelper = char;

#define IRAM_ATTR
#define F(str_literal) reinterpret_cast<const __FlashStringHelper *>(str_literal)

class String {
public:
    String() = default;
    String(const char *cstr) : data_(cstr ? cstr : "") {}
    String(const std::string &str) : data_(str) {}
    String(const __FlashStringHelper *flashStr) : data_(flashStr ? reinterpret_cast<const char *>(flashStr) : "") {}

    bool isEmpty() const { return data_.empty(); }
    const char *c_str() const { return data_.c_str(); }

    String &operator+=(const char *rhs) {
        if (rhs) {
            data_ += rhs;
        }
        return *this;
    }

    String &operator+=(const String &rhs) {
        data_ += rhs.data_;
        return *this;
    }

    String &operator=(const char *rhs) {
        data_ = rhs ? rhs : "";
        return *this;
    }

    bool operator==(const char *rhs) const {
        return data_ == (rhs ? rhs : "");
    }

    bool operator==(const String &rhs) const {
        return data_ == rhs.data_;
    }

    std::string toStdString() const { return data_; }

private:
    std::string data_;
};

unsigned long millis();
void setMillis(unsigned long value);
void advanceMillis(unsigned long delta);

int analogRead(uint8_t pin);
void setAnalogReadSequence(const std::vector<int> &values);

inline void pinMode(int, int) {}
inline void delay(unsigned long ms) { advanceMillis(ms); }
inline void delayMicroseconds(unsigned int) {}

class HardwareSerial {
public:
    explicit HardwareSerial(int) {}
    void begin(int, int = 0, int = -1, int = -1) {}
    void println(const char *) {}
    void print(const char *) {}
    bool available() const { return false; }
    char read() { return 0; }
    void write(char) {}
};

extern HardwareSerial Serial;

constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
constexpr int SERIAL_8N1 = 0;
