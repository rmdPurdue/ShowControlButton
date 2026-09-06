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
    constexpr uint32_t USER_BUTTON_HOLD_TIME_MS = 10000;
  }
}

namespace Flags {
  bool latchingRelay = false;
  bool triggerReset = false;
  bool triggerRelay = false;
  bool sendOSC = false;
  bool lightLED = false;
  bool configLockout = false;
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
EthernetServer server(80);

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

void serveFileFromFlash(EthernetClient& client, const char* filename);

void handleWebClient(EthernetClient client);

std::string getFormValue(String request, String key);

bool getCheckboxValue(const std::string& request, const std::string& checkboxName);

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
  oscManager.compileMessage(outMsg);

  // Start incoming UDP
  udp.begin(oscManager.getInPort());

  // Start web server
  server.begin();
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

  if(userButton.isHeldFor(Hardware::Inputs::USER_BUTTON_HOLD_TIME_MS)) {
    Flags::configLockout == !Flags::configLockout;
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

  if(Flags::configLockout == false) {
    // Handle web server and config requests
    EthernetClient client = server.available();
    if(client) {
      handleWebClient(client);
    }
  }  
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
  defaultOSCConfig.enabled = false;

  updateConfigSettings("ip", defaultNetConfig.ipAddress);
  updateConfigSettings("subnet", defaultNetConfig.subnet);
  updateConfigSettings("gatway", defaultNetConfig.gateway);
  updateConfigSettings("dhcp_en", defaultNetConfig.dhcpEnabled ? "true": "false");
  updateConfigSettings("mac", saveHexArrayToTomlString(defaultNetConfig.mac_address, sizeof(defaultNetConfig.mac_address)));
  updateConfigSettings("destination_ip", defaultOSCConfig.destination_ip);
  updateConfigSettings("destination_port", std::to_string(defaultOSCConfig.destination_port));
  updateConfigSettings("incoming_port", std::to_string(defaultOSCConfig.incoming_port));
  updateConfigSettings("incoming_address", defaultOSCConfig.incoming_address);
  updateConfigSettings("message", defaultOSCConfig.osc_address);
  updateConfigSettings("enabled", defaultOSCConfig.enabled ? "true" : "false");

  flashConfigSettings();
}

void updateCurrentSettingsAndFlash() {
  updateConfigSettings("ip", netManager.getConfiguration().ipAddress);
  updateConfigSettings("subnet", netManager.getConfiguration().subnet);
  updateConfigSettings("gateway", netManager.getConfiguration().gateway);
  updateConfigSettings("dhcp_en", netManager.getConfiguration().dhcpEnabled ? "true" : "false");
  updateConfigSettings("mac", saveHexArrayToTomlString(netManager.getConfiguration().mac_address, sizeof(netManager.getConfiguration().mac_address)));
  updateConfigSettings("destination_ip", oscManager.getDestinationIp());
  updateConfigSettings("desintation_port", std::to_string(oscManager.getOutPort()));
  updateConfigSettings("incoming_port", std::to_string(oscManager.getInPort()));
  updateConfigSettings("incoming_address", oscManager.getIncomingAddress());
  updateConfigSettings("message", oscManager.getMessage());
  updateConfigSettings("enabled", oscManager.getConfiguration().enabled ? "true" : "false");

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
  if(!targetIp.fromString(oscManager.getDestinationIp().c_str())) {
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

void serveFileFromFlash(EthernetClient& client, const char* filename) {
  File32 file = fatfs.open(filename, FILE_READ);

  if(!file) {
    client.println("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nFile Not Found");
    return;
  }

  // Send HTTP Header
  client.println("HTTP/1.1 200 OK");
  if(String(filename).endsWith(".html")) {
    client.println("Content-Type: text/html");
  } else if(String(filename).endsWith(".css")) {
    client.println("Content-Type: text/css");
  }
  client.println("Connection: close");
  client.println(); // Header end

  // Stream out using a chunk buffer for speed
  uint8_t buffer[64];
  while(file.available()) {
    int bytesRead = file.read(buffer, sizeof(buffer));
    client.write(buffer, bytesRead);
  }

  file.close();
}

void handleWebClient(EthernetClient client) {
  std::string requestBuffer = "";
  boolean currentLineIsBlank = true;

  while(client.connected()) {
    if(client.available()) {
      char c = client.read();
      requestBuffer += c;

      if(requestBuffer.length() > 500) {
        requestBuffer.erase(0, requestBuffer.size() - 300);
      }

      if(c == '\n' && currentLineIsBlank) {

        // Handle JSON form data population request
        if(requestBuffer.find("GET /api/settings") != std::string::npos) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println();

          client.println("{");
          client.printf("\"ip\":\"%s\",", netManager.getConfiguration().ipAddress.c_str());
          client.printf("\"subnet\":\"%s\",", netManager.getConfiguration().subnet.c_str());
          client.printf("\"gateway\":\"%s\",", netManager.getConfiguration().gateway.c_str());
          client.printf("\"destIp\":\"%s\",", oscManager.getDestinationIp().c_str());
          client.printf("\"destPort\":\"%s\",", oscManager.getOutPort());
          client.printf("\"message\":\"%s\",", oscManager.getMessage().c_str());
          client.printf("\"sendOSC\":\"%s\",", oscManager.getConfiguration().enabled ? "true" : "false");
          client.printf("\"recPort\":\"%s\",", oscManager.getInPort());
          client.println("}");

          break;
        }

        // Handle network form submission
        else if(requestBuffer.find("GET /submit-network") != std::string::npos) {
          NetworkConfig newNetworkConfig;
          newNetworkConfig.ipAddress = getFormValue(requestBuffer, "ip");
          newNetworkConfig.subnet = getFormValue(requestBuffer, "subnet");
          newNetworkConfig.gateway = getFormValue(requestBuffer, "gateway");

          netManager.updateConfiguration(newNetworkConfig);

          updateCurrentSettingsAndFlash();

          client.println("HTTP/1.1 303 See Other");
          client.println("Location: /?net_success = true");
          client.println("Connection: Close");
          client.println();
          break;
        }

        // Handle OSC form submission
        else if(requestBuffer.find("GET /submit-osc") != std::string::npos) {
          OSCConfig newOSCConfig;
          newOSCConfig.destination_ip = getFormValue(requestBuffer, "destIp");
          newOSCConfig.destination_port = std::stoi(getFormValue(requestBuffer, "destPort"));
          newOSCConfig.incoming_port = std::stoi(getFormValue(requestBuffer, "recPort"));
          oscManager.parseCommand(newOSCConfig, getFormValue(requestBuffer, "message"));
          newOSCConfig.enabled = getCheckboxValue(requestBuffer, "sendOSC");
          
          oscManager.updateConfiguration(newOSCConfig);

          updateCurrentSettingsAndFlash();

          client.println("HTTP/1.1 303 See Other");
          client.println("Location: /?osc_success = true");
          client.println("Connection: Close");
          client.println();
          break;
        }

        // Handle index page request
        else if(requestBuffer.find("GET /") != std::string::npos || requestBuffer.find("GET /index") != std::string::npos) {
          serveFileFromFlash(client, "/index.html");
          break;
        }

        // 404 Fallback
        else {
          client.println("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nNot Found");
        }
      }

      if(c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }
  delay(1);
  client.stop();
}

std::string getFormValue(std::string request, std::string key) {
  std::string tgtKey = key + "=";
  size_t keyIndex = request.find(tgtKey);
  if(keyIndex == std::string::npos) return "";

  size_t valStart = keyIndex + tgtKey.length();
  size_t valEnd = request.find('&', valStart);
  if(valEnd == std::string::npos) {
    valEnd = request.find(' ', valStart);
  }

  if(valEnd == std::string::npos) return "";

  std::string val = request.substr(valStart, valEnd - valStart);

  size_t pos;
  while((pos = val.find('+')) != std::string::npos) {
    val.replace(pos, 1, " ");
  }
  return val;
}

bool getCheckboxValue(const std::string& request, const std::string& checkboxName) {
  std::string tgtKey = checkboxName + "=";

  if(request.find(tgtKey) != std::string::npos) {
    return true;
  }
  return false;
}