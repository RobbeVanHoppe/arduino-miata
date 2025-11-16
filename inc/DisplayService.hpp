#pragma once

#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <algorithm>
#include <cstddef>
#include <cstring>

#include "Config.hpp"
#include "MenuPage.hpp"

class DisplayService {
public:
    DisplayService();

    void begin();
    void setPages(const MenuPageBase* const* pages, size_t count);
    void setActivePage(size_t index);
    void nextPage();
    size_t pageCount() const { return pageCount_; }
    void update(const SensorReadings& readings);

private:
    struct TextBounds {
        int16_t x = 0;
        int16_t y = 0;
        uint16_t w = 0;
        uint16_t h = 0;
    };

    void drawCenteredText(const char* text, int16_t centerY, uint8_t textSize, uint16_t color);
    void drawMenuValueText(const char* text);
    void drawPageIndicators(size_t selectedIndex);
    void drawMenuPageBackground(const MenuPageBase& page, size_t pageIndex);
    void resetValueTextBounds();

    Adafruit_GC9A01A tft_;
    const MenuPageBase* const* pages_ = nullptr;
    size_t pageCount_ = 0;
    size_t activePage_ = 0;

    bool backgroundDrawn_ = false;
    size_t lastPageIndex_ = 0;
    bool lastValueValid_ = false;
    char lastValueBuffer_[32];
    TextBounds lastValueBounds_;
    bool lastValueBoundsValid_ = false;
};
