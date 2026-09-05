#include "DigitalButton.h"

DigitalButton::DigitalButton(uint8_t pinNumber)
    : _pin(pinNumber), _pressStartTime(0), _isHeld(false), _isTriggered(false) {}

void DigitalButton::initialize() {
    pinMode(_pin, INPUT_PULLUP);
}

bool DigitalButton::isHeldFor(uint32_t durationMs) {
    bool isPressed = (digitalRead(_pin) == LOW);

    if(isPressed) {
        if(!_isHeld) {
          _pressStartTime = millis();
          _isHeld = true;
          _isTriggered = false;
        } else {
          if(millis() - _pressStartTime >= durationMs && !_isTriggered) {
            _isTriggered = true;
            return true;
          }
        }
      } else {
        _isHeld = false;
        _isTriggered = false;
      }

      return false;
}