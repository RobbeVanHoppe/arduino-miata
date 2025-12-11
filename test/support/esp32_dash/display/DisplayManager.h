#pragma once

#include "Arduino.h"

class DisplayManager {
public:
    void requestRefresh() { refreshRequested = true; }
    void setSuspended(bool suspended) { suspended_ = suspended; }
    bool isSuspended() const { return suspended_; }
    bool isReady() const { return true; }

    bool refreshRequested = false;

private:
    bool suspended_ = false;
};
