#include <Arduino.h>
#include "Adafruit_SPIFlash.h"
#include "Ethernet.h"
#include "EthernetUdp.h"
#include "SdFat.h"
#include "DigitalButton.h"
#include "DigitalOutput.h"
#include "NetworkManager.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace Hardware {
  // --- Outputs ---
  namespace Outputs {
    constexpr uint8_t LED_PIN = 12;
    constexpr uint8_t RELAY_PIN = 9;
    constexpr uint8_t LATCH_RESET_PIN = 13;
    constexpr uint8_t ETHERNET_CS_PIN = 10;
  }

  namespace Inputs {
    constexpr uint8_t RESET_BUTTON_PIN = 11;
    constexpr uint8_t USER_BUTTON_PIN = 6;

    constexpr uint32_t RESET_BUTTON_HOLD_TIME_MS = 5000;
    constexpr uint32_t USER_BUTTON_DEBOUNCE_TIME_MS = 50;
  }
}

// Define outputs
DigitalOutput LED(Hardware::Outputs::LED_PIN);
DigitalOutput relay(Hardware::Outputs::RELAY_PIN);
DigitalOutput latchReset(Hardware::Outputs::LATCH_RESET_PIN);

// Define inputs
DigitalButton userButton(Hardware::Inputs::USER_BUTTON_PIN);
DigitalButton resetButton(Hardware::Inputs::RESET_BUTTON_PIN);

// Define network manager
NetworkManager netManager;

// Set up internal flash memory read/write
#if defined(EXTERNAL_FLASH_USE_QSPI)
  Adafruit_FlashTransport_QSPI flashTransport;
#elif defined(EXTERNAL_FLASH_USE_SPI)
  Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_SPI)
#else
  #error "No internal flash transport defined for this board."
#endif

Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;


// Initialize OSC message variable

// Setup Ethernet FeatherWing
EthernetUDP udp;

// Initialize ethernet interface without DHCP

// Web server functions: default page; config page; form handler for config submission

// Update TOML file with config settings

// Start the web server

int myFunction(int, int);

void setup() {
  // Initialize the LED
  LED.initialize();

  //Initialize the relay
  relay.initialize();
  latchReset.initialize();

  // Initialize contestant button
  userButton.initialize();
  
  // Initialize reset button, including debouncing
  resetButton.initialize();

  // Load network configuration from storage
  flash.begin();
  fatfs.begin(&flash);

  File configFile = fatfs.open("settings.toml", FILE_READ);
  if(configFile) {
    std::string fileContent;
    fileContent.reserve(configFile.size());
    while(configFile.available()) {
      fileContent.push_back((char)configFile.read());
    }
    configFile.close();
    if(netManager.loadConfigFromToml(fileContent)) {
      Serial.println("Network config mapped successfully.");
    }
  }

  // Initalize Ethernet FeatherWing
  Ethernet.init(Hardware::Outputs::ETHERNET_CS_PIN);
  IPAddress ip, subnet, gateway;
  if(!ip.fromString(netManager.getConfiguration().ipAddress.c_str()) ||
     !subnet.fromString(netManager.getConfiguration().subnet.c_str()) ||
     !gateway.fromString(netManager.getConfiguration().gateway.c_str())) {
    Serial.println("Critical Error: one or more static IPs in TOML file are invalid.");
  } else {
    Ethernet.begin(netManager.getConfiguration().mac_address, ip, gateway, subnet);
  }
}

void loop() {
  // read button inputs

  // parse incoming osc messages

  // update led output

  // handle web server and config requests

  // TODO: is there merit in adding a "stage mode lockout" to shutdown the web server when the device is in use?
  // This could be a 30 second hold of the user button, and a 30 second hold to release it.
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}