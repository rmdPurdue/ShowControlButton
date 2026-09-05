#pragma once
#include <Arduino.h>

class DigitalButton {
private:
    uint8_t _pin;
    uint32_t _pressStartTime;
    bool _isHeld;
    bool _isTriggered;

public:
    explicit DigitalButton(uint8_t pinNumber);

    void initialize();

    bool isHeldFor(uint32_t durationMs);
};