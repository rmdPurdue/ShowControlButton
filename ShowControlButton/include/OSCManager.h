#pragma once
#include <Arduino.h>
#include <string>
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

struct OSCConfig {
    std::string destination_ip = "127.0.0.1";
    int destination_port = 8000;
    int incoming_port = 9000;
    std::string incoming_address = "/lamp/on";
    std::string osc_address = "/test";
    // I need an array of arguments
    // I need an array of argument types
    bool enabled = false;
};

class OSCManager {
private:
    OSCConfig activeConfig;

public:
    void updateConfiguration(const OSCConfig& newConfig);
    OSCConfig getConfiguration() const;
    const std::string& getOSCAddress() const;
    const std::string& getDesintationIp() const;
    const std::string& getIncomingAddress() const;
    int getOutPort() const;
    int getInPort() const;
    bool loadConfigFromToml(const std::string& tomlString);
};
