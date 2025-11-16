#include "DisplayService.hpp"

#include "SensorReadings.hpp"

DisplayService::DisplayService()
    : tft_(Config::TFT_CS_PIN, Config::TFT_DC_PIN, Config::TFT_RST_PIN) {
    std::memset(lastValueBuffer_, 0, sizeof(lastValueBuffer_));
}

void DisplayService::begin() {
    SPI.begin(Config::TFT_SCL_PIN, -1, Config::TFT_SDA_PIN, Config::TFT_CS_PIN);
    tft_.begin();
    tft_.setRotation(0);
    tft_.fillScreen(GC9A01A_BLACK);
    tft_.setTextWrap(false);
    if (Config::TFT_BACKLIGHT_PIN >= 0) {
        pinMode(Config::TFT_BACKLIGHT_PIN, OUTPUT);
        digitalWrite(Config::TFT_BACKLIGHT_PIN, HIGH);
    }
}

void DisplayService::setPages(const MenuPageBase* const* pages, size_t count) {
    pages_ = pages;
    pageCount_ = count;
    setActivePage(0);
}

void DisplayService::setActivePage(size_t index) {
    if (pageCount_ == 0) {
        return;
    }
    activePage_ = index % pageCount_;
    backgroundDrawn_ = false;
    lastValueValid_ = false;
    resetValueTextBounds();
}

void DisplayService::nextPage() {
    if (pageCount_ == 0) {
        return;
    }
    setActivePage(activePage_ + 1);
}

void DisplayService::update(const SensorReadings& readings) {
    if (!pages_ || pageCount_ == 0) {
        return;
    }
    const MenuPageBase& page = *pages_[activePage_];

    if (!backgroundDrawn_ || lastPageIndex_ != activePage_) {
        drawMenuPageBackground(page, activePage_);
        backgroundDrawn_ = true;
        lastPageIndex_ = activePage_;
        lastValueValid_ = false;
        resetValueTextBounds();
    }

    char valueBuffer[32] = {0};
    page.formatValue(readings, valueBuffer, sizeof(valueBuffer));

    if (!lastValueValid_ || std::strncmp(lastValueBuffer_, valueBuffer, sizeof(lastValueBuffer_)) != 0) {
        drawMenuValueText(valueBuffer);
        std::strncpy(lastValueBuffer_, valueBuffer, sizeof(lastValueBuffer_));
        lastValueBuffer_[sizeof(lastValueBuffer_) - 1] = '\0';
        lastValueValid_ = true;
    }
}

void DisplayService::drawCenteredText(const char* text, int16_t centerY, uint8_t textSize, uint16_t color) {
    if (!text) {
        return;
    }

    tft_.setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    tft_.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int16_t x = Config::DISPLAY_CENTER_X - static_cast<int16_t>(w / 2);
    int16_t y = centerY - static_cast<int16_t>(h / 2);
    tft_.setCursor(x, y);
    tft_.setTextColor(color);
    tft_.print(text);
}

void DisplayService::drawMenuValueText(const char* text) {
    if (!text) {
        return;
    }

    tft_.setTextSize(Config::VALUE_TEXT_SIZE);
    int16_t x1, y1;
    uint16_t w, h;
    tft_.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    TextBounds bounds;
    bounds.w = w;
    bounds.h = h;
    bounds.x = Config::DISPLAY_CENTER_X - static_cast<int16_t>(bounds.w / 2);
    bounds.y = Config::VALUE_TEXT_Y - static_cast<int16_t>(bounds.h / 2);

    auto clearRegion = [&](int16_t x, int16_t y, int16_t width, int16_t height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        tft_.fillRect(x, y, width, height, GC9A01A_BLACK);
    };

    if (lastValueBoundsValid_) {
        const int16_t clearLeft = std::min(bounds.x, lastValueBounds_.x) - Config::VALUE_TEXT_PADDING;
        const int16_t clearTop = std::min(bounds.y, lastValueBounds_.y) - Config::VALUE_TEXT_PADDING;
        const int16_t clearRight = std::max(static_cast<int16_t>(bounds.x + bounds.w),
                                            static_cast<int16_t>(lastValueBounds_.x + lastValueBounds_.w)) + Config::VALUE_TEXT_PADDING;
        const int16_t clearBottom = std::max(static_cast<int16_t>(bounds.y + bounds.h),
                                             static_cast<int16_t>(lastValueBounds_.y + lastValueBounds_.h)) + Config::VALUE_TEXT_PADDING;
        clearRegion(clearLeft,
                    clearTop,
                    clearRight - clearLeft,
                    clearBottom - clearTop);
    } else {
        clearRegion(bounds.x - Config::VALUE_TEXT_PADDING,
                    bounds.y - Config::VALUE_TEXT_PADDING,
                    static_cast<int16_t>(bounds.w) + Config::VALUE_TEXT_PADDING * 2,
                    static_cast<int16_t>(bounds.h) + Config::VALUE_TEXT_PADDING * 2);
    }

    tft_.setCursor(bounds.x, bounds.y);
    tft_.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
    tft_.print(text);

    lastValueBounds_ = bounds;
    lastValueBoundsValid_ = true;
}

void DisplayService::drawPageIndicators(size_t selectedIndex) {
    if (pageCount_ <= 1) {
        return;
    }

    const int16_t indicatorY = Config::DISPLAY_CENTER_Y + Config::DISPLAY_RADIUS - 25;
    const int16_t spacing = 18;
    const int16_t totalWidth = (pageCount_ - 1) * spacing;
    const int16_t startX = Config::DISPLAY_CENTER_X - totalWidth / 2;

    for (size_t i = 0; i < pageCount_; ++i) {
        uint16_t color = (i == selectedIndex) ? GC9A01A_WHITE : GC9A01A_DARKGREY;
        uint8_t radius = (i == selectedIndex) ? 5 : 3;
        tft_.fillCircle(startX + static_cast<int16_t>(i) * spacing, indicatorY, radius, color);
    }
}

void DisplayService::drawMenuPageBackground(const MenuPageBase& page, size_t pageIndex) {
    tft_.fillScreen(GC9A01A_BLACK);
    tft_.fillCircle(Config::DISPLAY_CENTER_X, Config::DISPLAY_CENTER_Y, Config::DISPLAY_RADIUS, GC9A01A_BLACK);
    tft_.drawCircle(Config::DISPLAY_CENTER_X, Config::DISPLAY_CENTER_Y, Config::DISPLAY_RADIUS, GC9A01A_DARKGREY);
    tft_.drawCircle(Config::DISPLAY_CENTER_X, Config::DISPLAY_CENTER_Y, Config::DISPLAY_RADIUS - 2, GC9A01A_DARKGREY);

    drawCenteredText(page.title(), Config::DISPLAY_CENTER_Y - 70, Config::TITLE_TEXT_SIZE, GC9A01A_CYAN);

    if (page.units() && page.units()[0] != '\0') {
        drawCenteredText(page.units(), Config::DISPLAY_CENTER_Y + 45, Config::UNIT_TEXT_SIZE, GC9A01A_LIGHTGREY);
    }

    drawPageIndicators(pageIndex);
}

void DisplayService::resetValueTextBounds() {
    lastValueBoundsValid_ = false;
}
