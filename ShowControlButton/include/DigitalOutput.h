#pragma once
#include <Arduino.h>

class DigitalOutput {
private:
    uint8_t _pin;

public:
    explicit DigitalOutput(uint8_t pinNumber);

    void initialize();

    void setState(bool state);
};