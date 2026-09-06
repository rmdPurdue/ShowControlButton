#pragma once
#include <Arduino.h>
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include "OSCMessage.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

using OscArg = std::variant<int, float, bool, std::string>;

struct OSCConfig {
    std::string destination_ip = "127.0.0.1";
    int destination_port = 8000;
    int incoming_port = 9000;
    std::string incoming_address = "/lamp/on";
    std::string osc_address = "/test";
    std::vector<OscArg> args;
    std::vector<std::string> types;
    bool enabled = false;
};

class OSCManager {
private:
    OSCConfig activeConfig;
    bool isInt(const std::string& s);
    bool isFloat(const std::string& s);

public:
    void updateConfiguration(const OSCConfig& newConfig);
    OSCConfig getConfiguration() const;
    const std::string& getOSCAddress() const;
    const std::string& getDestinationIp() const;
    const std::string& getIncomingAddress() const;
    int getOutPort() const;
    int getInPort() const;
    bool loadConfigFromToml(const std::string& tomlString);
    void parseCommand(OSCConfig& config, const std::string& rawLine);
    OSCMessage compileMessage(OSCMessage& msg);
    const std::string& getMessage() const;
};
