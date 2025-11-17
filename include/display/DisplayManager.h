#pragma once

#include <Arduino.h>
#include <Adafruit_GC9A01A.h>
#include <memory>
#include <vector>

#include "display/DisplayPage.h"

struct DisplayConfig {
    int8_t csPin = 5;
    int8_t dcPin = 16;
    int8_t rstPin = 17;
    int8_t sdaPin = 23;
    int8_t sclPin = 18;
    int8_t backlightPin = -1;  // optional
    uint8_t rotation = 0;
    uint16_t backgroundColor = 0xffff;  // white
    uint32_t refreshIntervalMs = 1000;
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
    size_t currentPageIndex() const { return _currentPage; }

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
