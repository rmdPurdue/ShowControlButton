#pragma once
#include <Arduino.h>
#include <string>
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

struct NetworkConfig {
    uint8_t mac_address[6];
    std::string ipAddress;
    std::string subnet;
    std::string gateway;
    std::string dns;
    bool dhcpEnabled = false;
    bool isConnected = false;
};

class NetworkManager {
private:
    NetworkConfig activeConfig;
    void parseMacAddress(const std::string& macStr, uint8_t* macBytes);

public:
    void updateConfiguration(const NetworkConfig& newConfig);
    NetworkConfig getConfiguration() const;
    bool loadConfigFromToml(const std::string& tomlString);
};