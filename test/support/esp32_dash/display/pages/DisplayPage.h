#pragma once

#include "Adafruit_GC9A01A.h"

class DisplayPage {
public:
    virtual ~DisplayPage() = default;
    virtual void onEnter(Adafruit_GC9A01A &display) { (void) display; }
    virtual void onExit(Adafruit_GC9A01A &display) { (void) display; }
    virtual void render(Adafruit_GC9A01A &display) = 0;
};

