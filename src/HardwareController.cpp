#include "HardwareController.hpp"

void HardwareController::begin() {
    pinMode(Config::LIGHTS_PIN, OUTPUT);
    pinMode(Config::WINDOWS_PIN, OUTPUT);
    digitalWrite(Config::LIGHTS_PIN, LOW);
    digitalWrite(Config::WINDOWS_PIN, LOW);
}

void HardwareController::setLights(bool on) {
    digitalWrite(Config::LIGHTS_PIN, on ? HIGH : LOW);
    Serial.printf("Lights %s\n", on ? "ON" : "OFF");
}

void HardwareController::setWindows(bool up) {
    digitalWrite(Config::WINDOWS_PIN, up ? HIGH : LOW);
    Serial.printf("Windows %s\n", up ? "UP" : "DOWN");
}
