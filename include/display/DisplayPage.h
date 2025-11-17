#pragma once

#include <Arduino.h>
#include <Adafruit_GC9A01A.h>

/**
 * Base interface for every drawable page on the GC9A01 display.
 *
 * Extend this class to implement custom pages. Override \c render to draw
 * your page and optionally \c onEnter / \c onExit to perform setup or cleanup
 * whenever the page becomes active or inactive.
 */
class DisplayPage {
public:
    virtual ~DisplayPage() = default;

    virtual void onEnter(Adafruit_GC9A01A &display) { (void) display; }
    virtual void onExit(Adafruit_GC9A01A &display) { (void) display; }
    virtual void render(Adafruit_GC9A01A &display) = 0;
};
