#include "DigitalOutput.h"

DigitalOutput::DigitalOutput(uint8_t pinNumber)
    : _pin(pinNumber) {}

void DigitalOutput::initialize() {
    pinMode(_pin, OUTPUT);
}

void DigitalOutput::setState(bool state) {
    digitalWrite(_pin, state ? HIGH : LOW);
}