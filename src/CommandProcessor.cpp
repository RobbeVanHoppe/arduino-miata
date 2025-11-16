#include "CommandProcessor.hpp"

CommandProcessor::CommandProcessor(HardwareController& hardware, DisplayService& display)
    : hardware_(hardware), display_(display) {}

void CommandProcessor::handleCommand(const std::string& command) {
    if (command == "LIGHTS:ON") {
        hardware_.setLights(true);
    } else if (command == "LIGHTS:OFF") {
        hardware_.setLights(false);
    } else if (command == "WINDOWS:UP") {
        hardware_.setWindows(true);
    } else if (command == "WINDOWS:DOWN") {
        hardware_.setWindows(false);
    } else if (command == "MENU") {
        display_.nextPage();
    } else {
        Serial.println("Unknown command");
    }
}
