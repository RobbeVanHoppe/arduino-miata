#pragma once

#include <cstddef>

#include "SensorReadings.hpp"

class MenuPageBase {
public:
    constexpr MenuPageBase(const char* title, const char* units)
        : title_(title), units_(units) {}
    virtual ~MenuPageBase() = default;

    const char* title() const { return title_; }
    const char* units() const { return units_; }

    virtual void formatValue(const SensorReadings& readings, char* buffer, size_t len) const = 0;

private:
    const char* title_;
    const char* units_;
};

template <typename Formatter>
class MenuPage : public MenuPageBase {
public:
    constexpr MenuPage(const char* title, const char* units, Formatter formatter)
        : MenuPageBase(title, units), formatter_(formatter) {}

    void formatValue(const SensorReadings& readings, char* buffer, size_t len) const override {
        formatter_(readings, buffer, len);
    }

private:
    Formatter formatter_;
};
