//
// Created by robbe on 06/12/2025.
//
#pragma once

class gpsHandler {
public:
    void begin();
    void sendCommand(char* cmd);
    char* readBuffer();

};