#include <Arduino.h>
#include <sstream>
#include <iomanip>
#include "Adafruit_SPIFlash.h"
#include "Ethernet.h"
#include "EthernetUdp.h"
#include "OSCMessage.h"
#include "SdFat.h"
#include "DigitalButton.h"
#include "DigitalOutput.h"
#include "NetworkManager.h"
#include "OSCManager.h"
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

namespace Flags {
  bool latchingRelay = false;
  bool triggerReset = false;
  bool triggerRelay = false;
  bool sendOSC = false;
  bool lightLED = false;
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
std::string configFileContent;

// Define outgoing OSC message variable, OSC Manager
OSCManager oscManager;
OSCMessage outMsg;

// Setup Ethernet FeatherWing
EthernetUDP udp;
unsigned long lastLinkCheckTime = 0;
const unsigned long linkCheckInterval = 1000;

// Web server functions: default page; config page; form handler for config submission

// Start the web server

void updateConfigSettings(const std::string& key, const std::string& value); 

bool flashConfigSettings();

void resetToDefaults();

std::string saveHexArrayToTomlString(const uint8_t* array, size_t length);

bool loadHardwareConfig(const std::string& tomlString);

void cyclePin(uint8_t);

void sendOSCMessage();

void routeLEDControl(OSCMessage &msg);

void setup() {
  // Initialize the LED
  LED.initialize();

  // Initialize the relay
  relay.initialize();
  latchReset.initialize();

  // Initialize contestant button
  userButton.initialize();
  
  // Initialize reset button, including debouncing
  resetButton.initialize();

  // Initialize relay outputs
  digitalWrite(Hardware::Outputs::RELAY_PIN, LOW);
  digitalWrite(Hardware::Outputs::LATCH_RESET_PIN, LOW);

  // Initialize LED output
  digitalWrite(Hardware::Outputs::LED_PIN, LOW);

  // Load configuration settings from storage
  flash.begin();
  fatfs.begin(&flash);

  File configFile = fatfs.open("settings.toml", FILE_READ);
  if(configFile) {
    configFileContent.reserve(configFile.size());
    while(configFile.available()) {
      configFileContent.push_back((char)configFile.read());
    }
    configFile.close();

    if(netManager.loadConfigFromToml(configFileContent)) {
      Serial.println("Network config mapped successfully.");
    }

    if(oscManager.loadConfigFromToml(configFileContent)) {
      Serial.println("OSC config mapped successfully.");
    }

    if(loadHardwareConfig(configFileContent)) {
      Serial.println("Hardware config loaded successfully.");
    }
  }

  // Initalize Ethernet FeatherWing
  netManager.init(netManager.getConfiguration(), Hardware::Outputs::ETHERNET_CS_PIN);

  // Initialize outgoing OSC Message
  outMsg.setAddress(oscManager.getOSCAddress().c_str());

  // Start incoming UDP
  udp.begin(oscManager.getInPort());
}

void loop() {
  // Check network connection
  netManager.updateConnectionStatus();

  // Check status flags
  if(Flags::triggerReset) {
    resetToDefaults();
    Flags::triggerReset = false;
  }

  if(Flags::triggerRelay) {
    cyclePin(Hardware::Outputs::RELAY_PIN);
    if(Flags::latchingRelay) {
      cyclePin(Hardware::Outputs::LATCH_RESET_PIN);
    }
    Flags::triggerRelay = false;
  }
  
  if(Flags::sendOSC) {
    if(netManager.isNetworkReady()) {
        sendOSCMessage();
    } else {
      Serial.println("Network not available.");
    }
    Flags::sendOSC = false;
  }

  // Check buttons
  if(resetButton.isHeldFor(Hardware::Inputs::RESET_BUTTON_HOLD_TIME_MS)) {
    Flags::triggerReset = true;
  }

  if(userButton.isHeldFor(Hardware::Inputs::USER_BUTTON_DEBOUNCE_TIME_MS)) {
    Flags::triggerRelay = true;
    
    if(oscManager.getConfiguration().enabled) {
      Flags::sendOSC = true;
    }
  } else {
    Flags::triggerRelay = false;
    Flags::sendOSC = false;
  }
  
  // parse incoming osc messages
  if(netManager.isNetworkReady()) {
    OSCMessage inMsg;
    int size = udp.parsePacket();
    if(size > 0) {
      while(size--) {
        inMsg.fill(udp.read());
      }

      if(!inMsg.hasError()) {
        inMsg.dispatch(oscManager.getIncomingAddress().c_str(), routeLEDControl);
      } else {
        OSCErrorCode error = inMsg.getError();
        Serial.print("Error: OSC Packet error code: ");
        Serial.println(error);
      }
    }
  }

  // handle web server and config requests
  // Update outgoing osc message data

  // TODO: is there merit in adding a "stage mode lockout" to shutdown the web server when the device is in use?
  // This could be a 30 second hold of the user button, and a 30 second hold to release it.
  
}

void updateConfigSettings(const std::string& key, const std::string& value) {
  size_t keyPos = configFileContent.find(key + " =");
  if(keyPos == std::string::npos) {
    keyPos = configFileContent.find(key + "=");
  }
  
  std::string newLine = key + " = " + value + "\n";

  if(keyPos != std::string::npos) {
    // Key exists. find the end of the line and replace it.
    size_t endOfLine = configFileContent.find("\n", keyPos);
    if(endOfLine == std::string::npos) {
      endOfLine = configFileContent.length();
    } else {
      endOfLine += 1;
    }
    configFileContent.replace(keyPos, endOfLine - keyPos, newLine);
  } else {
    // Key doesn't exist. Append to end.
    if(!configFileContent.empty() && configFileContent.back() != '\n') {
      configFileContent.push_back('\n');
    }
    configFileContent.append(newLine);
  }
}

bool flashConfigSettings() {
  File writeHandle = fatfs.open("settings.toml", FILE_WRITE);
  if(writeHandle) {
    writeHandle.write(configFileContent.c_str(), configFileContent.length());
    writeHandle.close();
    Serial.println("Batch settings successfully committed to flash.");
    return true;
  } else {
    Serial.println("Error: Failed to write config file to storage.");
    return false;
  }
}

void resetToDefaults() {
  NetworkConfig defaultNetConfig;
  defaultNetConfig.ipAddress = "192.168.1.10";
  defaultNetConfig.subnet = "255.255.254.0";
  defaultNetConfig.gateway = "192.168.1.1";
  defaultNetConfig.dhcpEnabled = false;
  uint8_t dummy[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  memcpy(defaultNetConfig.mac_address, dummy, 6);

  OSCConfig defaultOSCConfig;
  defaultOSCConfig.destination_ip = "127.0.0.1";
  defaultOSCConfig.destination_port = 8000;
  defaultOSCConfig.incoming_port = 9000;
  defaultOSCConfig.incoming_address = "/lamp/on";
  defaultOSCConfig.osc_address = "/test";
    // I need an array of arguments
    // I need an array of argument types
  defaultOSCConfig.enabled = false;

  updateConfigSettings("ip", defaultNetConfig.ipAddress);
  updateConfigSettings("subnet", defaultNetConfig.subnet);
  updateConfigSettings("gatway", defaultNetConfig.gateway);
  updateConfigSettings("dhcp_en", defaultNetConfig.dhcpEnabled ? "true": "false");
  updateConfigSettings("ip", saveHexArrayToTomlString(defaultNetConfig.mac_address, sizeof(defaultNetConfig.mac_address)));
  updateConfigSettings("destination_ip", defaultOSCConfig.destination_ip);
  updateConfigSettings("destination_port", std::to_string(defaultOSCConfig.destination_port));
  updateConfigSettings("incoming_port", std::to_string(defaultOSCConfig.incoming_port));
  updateConfigSettings("incoming_address", defaultOSCConfig.incoming_address);
  updateConfigSettings("message", defaultOSCConfig.osc_address);
  updateConfigSettings("enabled", defaultOSCConfig.enabled ? "true" : "false");

  flashConfigSettings();
}

std::string saveHexArrayToTomlString(const uint8_t* array, size_t length) {
  if(length == 0) return "";

  std::stringstream ss;
  
  for(size_t i = 0; i < length; ++i) {
    ss << std::setfill('0') << std::setw(2)
       << std::hex << std::uppercase << (int)array[i];

    if(i < length - 1) {
      ss << ":";
    }
  }

  return ss.str();
}

bool loadHardwareConfig(const std::string& tomlString) {
    toml::parse_result result = toml::parse(tomlString);
    if(!result) return false; // Parse error

    const toml::table& config = result.table();
    Flags::latchingRelay = config["hardware"]["latching"].value_or(false);
}

void cyclePin(uint8_t pin) {
    digitalWrite(pin, HIGH);
    delay(10);
    digitalWrite(pin, LOW);
}

void sendOSCMessage() {
  IPAddress targetIp;
  if(!targetIp.fromString(oscManager.getDesintationIp().c_str())) {
    Serial.println("Error: could not parse destination IP Address.");
    return;
  }

  udp.beginPacket(targetIp, oscManager.getOutPort());
  outMsg.send(udp);
  udp.endPacket();
}

void routeLEDControl(OSCMessage &msg) {
  bool lampState = false;
  if(msg.isBoolean(0)) {
    lampState = msg.getBoolean(0);
  } else if(msg.isInt(0)) {
    lampState = (msg.getInt(0) == 1);
  } else if(msg.isFloat(0)) {
    lampState = (msg.getFloat(0) >= 0.5f);
  }

  if(lampState) {
    Flags::lightLED = true;
  } else {
    Flags::lightLED = false;
  }
}