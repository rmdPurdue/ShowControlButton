#pragma once
#include <Arduino.h>
#include <string>
#include "Ethernet.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

struct NetworkConfig {
    uint8_t mac_address[6];
    std::string ipAddress;
    std::string subnet;
    std::string gateway;
    std::string dns;
    bool dhcpEnabled = false;
};

class NetworkManager {
private:
    NetworkConfig activeConfig;
    EthernetLinkStatus lastLinkState = Unknown;
    unsigned long lastCheckTime = 0;
    const unsigned long checkInterval = 0;
    bool isConnected = false;
    bool connectHardware();
    void parseMacAddress(const std::string& macStr, uint8_t* macBytes);

public:
    void init(const NetworkConfig& config, uint8_t pin);
    void updateConnectionStatus();
    void updateConfiguration(const NetworkConfig& newConfig);
    NetworkConfig getConfiguration() const;
    bool loadConfigFromToml(const std::string& tomlString);
    bool isNetworkReady() const;
};