//
// Created by robbe on 10/01/2026.
//
#include "esp32_dash/GPS/gpsHandler.h"

void gpsHandler::begin() {

}

char* gpsHandler::readBuffer() {
    return new char('x');
}

void gpsHandler::sendCommand(char *cmd) {}