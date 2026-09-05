#include <Arduino.h>
#include "DigitalButton.h"
#include "DigitalOutput.h"

namespace Hardware {
  // --- Outputs ---
  namespace Outputs {
    constexpr uint8_t LED_PIN = 12;
    constexpr uint8_t RELAY_PIN = 9;
    constexpr uint8_t LATCH_RESET_PIN = 13;
  }

  namespace Inputs {
    constexpr uint8_t RESET_BUTTON_PIN = 11;
    constexpr uint8_t USER_BUTTON_PIN = 6;

    constexpr uint32_t RESET_BUTTON_HOLD_TIME_MS = 5000;
    constexpr uint32_t USER_BUTTON_DEBOUNCE_TIME_MS = 50;
  }
}

// Default network configuration

// Initialize OSC message variable

// Setup Ethernet FeatherWing

// Initialize ethernet interface without DHCP

// Web server functions: default page; config page; form handler for config submission

// Update TOML file with config settings

// Start the web server

// Utilize multithreading to let web server run in tandem to user operation


// put function declarations here:
int myFunction(int, int);

void setup() {
  // Initialize the LED
  DigitalOutput LED(Hardware::Outputs::LED_PIN);
  LED.initialize();

  //Initialize the relay
  DigitalOutput relay(Hardware::Outputs::RELAY_PIN);
  relay.initialize();
  DigitalOutput latchReset(Hardware::Outputs::LATCH_RESET_PIN);
  latchReset.initialize();

  // Initialize contestant button
  DigitalButton userButton(Hardware::Inputs::USER_BUTTON_PIN);
  userButton.initialize();
  
  // Initialize reset button, including debouncing
  DigitalButton resetButton(Hardware::Inputs::RESET_BUTTON_PIN);
  resetButton.initialize();
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}