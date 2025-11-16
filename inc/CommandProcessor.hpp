#pragma once

#include <string>

#include "DisplayService.hpp"
#include "HardwareController.hpp"

class CommandProcessor {
public:
    CommandProcessor(HardwareController& hardware, DisplayService& display);

    void handleCommand(const std::string& command);

private:
    HardwareController& hardware_;
    DisplayService& display_;
};
