#pragma once

#include "esp32_dash/display/DisplayPage.h"

class TachPage : public DisplayPage {
public:
    void setTitle(const String &title) { title_ = title; }
    void setRpm(float rpm) { rpm_ = rpm; }
    void setStatusMessage(const String &status) { statusMessage_ = status; }

    void onEnter(Adafruit_GC9A01A &display) override { (void) display; }
    void render(Adafruit_GC9A01A &display) override { (void) display; }

    String title_;
    float rpm_ = 0.0f;
    String statusMessage_;
};

