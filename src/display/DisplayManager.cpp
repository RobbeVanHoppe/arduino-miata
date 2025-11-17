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

    if (!_pages.empty()) {
        _pages[_currentPage]->onEnter(*_display);
    }

    return true;
}

void DisplayManager::loop() {
    if (!_initialized || !_display) {
        return;
    }

    const uint32_t now = millis();
    const bool intervalElapsed =
            _config.refreshIntervalMs > 0 &&
            (now - _lastRender) >= _config.refreshIntervalMs;

    if (_dirty || intervalElapsed) {
        if (_pages.empty()) {
            drawPlaceholder();
        } else {
            _pages[_currentPage]->render(*_display);
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
