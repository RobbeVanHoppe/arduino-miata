#pragma once

#include <Arduino.h>
#include <Adafruit_GC9A01A.h>
#include <memory>
#include <vector>

#include "display/DisplayPage.h"

struct DisplayConfig {
    int8_t csPin = 5;
    int8_t dcPin = 2;
    int8_t rstPin = 4;
    int8_t backlightPin = -1;  // optional
    uint8_t rotation = 0;
    uint16_t backgroundColor = 0x0000;  // black
    uint32_t refreshIntervalMs = 250;
    uint16_t width = 240;
    uint16_t height = 240;
};

class DisplayManager {
public:
    explicit DisplayManager(const DisplayConfig &config = {});

    bool begin();
    void loop();

    void addPage(DisplayPage *page);
    void nextPage();
    void previousPage();
    void showPage(size_t index);

    void requestRefresh();

    Adafruit_GC9A01A *display();
    bool isReady() const { return _initialized; }

private:
    void drawPlaceholder();

    DisplayConfig _config;
    std::unique_ptr<Adafruit_GC9A01A> _display;
    std::vector<DisplayPage *> _pages;
    size_t _currentPage = 0;
    uint32_t _lastRender = 0;
    bool _initialized = false;
    bool _dirty = true;
};
