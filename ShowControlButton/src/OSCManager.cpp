#include "OSCManager.h"

void OSCManager::updateConfiguration(const OSCConfig& newConfig) {
    activeConfig = newConfig;
}

OSCConfig OSCManager::getConfiguration() const {
    return activeConfig;
}

const std::string& OSCManager::getOSCAddress() const {
    return activeConfig.osc_address;
}

const std::string& OSCManager::getDesintationIp() const {
    return activeConfig.destination_ip;
}

const std::string& OSCManager::getIncomingAddress() const {
    return activeConfig.incoming_address;
}

int OSCManager::getOutPort() const {
    return activeConfig.destination_port;
}

bool OSCManager::loadConfigFromToml(const std::string& tomlString) {
    toml::parse_result result = toml::parse(tomlString);
    if(!result) return false; // Parse error

    const toml::table& config = result.table();
    activeConfig.destination_ip = config["osc"]["destination_ip"].value_or("127.0.0.1");
    activeConfig.destination_port = std::stoi(config["osc"]["destination_port"].value_or("8000"));
    activeConfig.incoming_port = std::stoi(config["osc"]["incoming_port"].value_or("9000"));
    // I need to read the array of arguments, probably delimited by commas.
    // I need to read the array of argument types, probably delimited by commas.
    activeConfig.enabled = config["osc"]["enabled"].value_or(true);
}
