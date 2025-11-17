#include "display/DisplayManager.h"

#include <SPI.h>

DisplayManager::DisplayManager(const DisplayConfig &config) : _config(config) {}

bool DisplayManager::begin() {
    if (_initialized) {
        return true;
    }

    if (_config.backlightPin >= 0) {
        pinMode(_config.backlightPin, OUTPUT);
        digitalWrite(_config.backlightPin, HIGH);
    }

    _display.reset(
            new Adafruit_GC9A01A(_config.csPin, _config.dcPin, _config.rstPin)
    );
    if (!_display) {
        Serial.println("Failed to allocate GC9A01A display");
        return false;
    } else {
        Serial.println("Display set up");
    }

    _display->begin();
    _display->setRotation(_config.rotation);
    _display->fillScreen(_config.backgroundColor);

    _initialized = true;
    _dirty = true;
    _lastRender = millis();
    _suspended = false;
    _transientMessage.active = false;

    if (!_pages.empty()) {
        _pages[_currentPage]->onEnter(*_display);
    }

    return true;
}

void DisplayManager::loop() {
    if (!_initialized || !_display) {
        return;
    }
    if (_suspended) {
        return;
    }

    const uint32_t now = millis();
    if (_transientMessage.active &&
        _transientMessage.durationMs > 0 &&
        (now - _transientMessage.shownAt) >= _transientMessage.durationMs) {
        _transientMessage.active = false;
        _dirty = true;
    }

    const bool intervalElapsed =
            _config.refreshIntervalMs > 0 &&
            (now - _lastRender) >= _config.refreshIntervalMs;

    if (_dirty || intervalElapsed) {
        if (_pages.empty()) {
            drawPlaceholder();
        } else {
            _pages[_currentPage]->render(*_display);
        }
        if (_transientMessage.active) {
            drawTransientOverlay();
        }
        _lastRender = now;
        _dirty = false;
    }
}

void DisplayManager::addPage(DisplayPage *page) {
    if (!page) {
        return;
    }
    _pages.push_back(page);
    if (_pages.size() == 1 && _initialized && _display) {
        page->onEnter(*_display);
        _dirty = true;
    }
}

void DisplayManager::nextPage() {
    if (_pages.size() <= 1 || !_display) {
        return;
    }
    _pages[_currentPage]->onExit(*_display);
    _currentPage = (_currentPage + 1) % _pages.size();
    _pages[_currentPage]->onEnter(*_display);
    _dirty = true;
}

void DisplayManager::previousPage() {
    if (_pages.size() <= 1 || !_display) {
        return;
    }
    _pages[_currentPage]->onExit(*_display);
    if (_currentPage == 0) {
        _currentPage = _pages.size() - 1;
    } else {
        _currentPage--;
    }
    _pages[_currentPage]->onEnter(*_display);
    _dirty = true;
}

void DisplayManager::showPage(size_t index) {
    if (index >= _pages.size() || !_display) {
        return;
    }
    if (index == _currentPage) {
        _dirty = true;
        return;
    }
    _pages[_currentPage]->onExit(*_display);
    _currentPage = index;
    _pages[_currentPage]->onEnter(*_display);
    _dirty = true;
}

void DisplayManager::requestRefresh() {
    _dirty = true;
}

void DisplayManager::setSuspended(bool suspended) {
    if (_suspended == suspended) {
        return;
    }
    _suspended = suspended;
    if (_config.backlightPin >= 0) {
        digitalWrite(_config.backlightPin, suspended ? LOW : HIGH);
    }
    if (suspended) {
        _transientMessage.active = false;
    } else {
        _dirty = true;
    }
}

void DisplayManager::showTransientMessage(const String &message,
                                          uint32_t durationMs,
                                          uint16_t textColor,
                                          uint16_t backgroundColor) {
    if (!_initialized || !_display || _suspended) {
        return;
    }
    if (message.isEmpty()) {
        _transientMessage.active = false;
        _dirty = true;
        return;
    }
    _transientMessage.text = message;
    _transientMessage.shownAt = millis();
    _transientMessage.durationMs = durationMs;
    _transientMessage.textColor = textColor;
    _transientMessage.backgroundColor = backgroundColor;
    _transientMessage.active = true;
    _dirty = true;
}

Adafruit_GC9A01A *DisplayManager::display() {
    return _display.get();
}

void DisplayManager::drawPlaceholder() {
    Serial.println("Drawing placeholder");
    if (!_display) {
        return;
    }
    _display->fillScreen(_config.backgroundColor);
    _display->setTextColor(0xFFFF);
    _display->setTextSize(2);
    _display->setCursor(10, _config.height / 2);
    _display->println(F("Miata"));
    delay(3000);
}

void DisplayManager::drawTransientOverlay() {
    if (!_display || !_transientMessage.active || _transientMessage.text.isEmpty()) {
        return;
    }

    Adafruit_GC9A01A &display = *_display;
    display.setTextWrap(false);
    const uint8_t textSize = 2;
    display.setTextSize(textSize);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(_transientMessage.text.c_str(), 0, 0, &x1, &y1, &w, &h);

    constexpr int16_t kPaddingX = 12;
    constexpr int16_t kPaddingY = 8;
    int16_t boxWidth = static_cast<int16_t>(w) + (kPaddingX * 2);
    const int16_t maxWidth = static_cast<int16_t>(display.width()) - 20;
    if (boxWidth > maxWidth) {
        boxWidth = maxWidth;
    }
    if (boxWidth < 40) {
        boxWidth = 40;
    }
    int16_t boxHeight = static_cast<int16_t>(h) + (kPaddingY * 2);
    if (boxHeight < 20) {
        boxHeight = 20;
    }
    if (boxHeight > static_cast<int16_t>(display.height()) - 20) {
        boxHeight = static_cast<int16_t>(display.height()) - 20;
    }
    int16_t boxX = (static_cast<int16_t>(display.width()) - boxWidth) / 2;
    if (boxX < 10) {
        boxX = 10;
    }
    int16_t boxY = static_cast<int16_t>(display.height()) - boxHeight - 20;
    if (boxY < 10) {
        boxY = 10;
    }

    display.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 10, _transientMessage.backgroundColor);
    display.drawRoundRect(boxX, boxY, boxWidth, boxHeight, 10, _transientMessage.textColor);

    display.setTextColor(_transientMessage.textColor, _transientMessage.backgroundColor);
    display.setCursor(boxX + kPaddingX, boxY + kPaddingY);
    display.print(_transientMessage.text);
}
