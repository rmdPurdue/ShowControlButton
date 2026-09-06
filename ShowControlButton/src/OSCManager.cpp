#include "OSCManager.h"

bool OSCManager::isInt(const std::string& s) {
    if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return;
    char *p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

bool OSCManager::isFloat(const std::string& s) {
    char* endptr = nullptr;
    strtod(s.c_str(), &endptr);
    return endptr != s.c_str() && *endptr == '\0';
}

void OSCManager::updateConfiguration(const OSCConfig& newConfig) {
    activeConfig = newConfig;
}

OSCConfig OSCManager::getConfiguration() const {
    return activeConfig;
}

const std::string& OSCManager::getOSCAddress() const {
    return activeConfig.osc_address;
}

const std::string& OSCManager::getDestinationIp() const {
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
    parseCommand(activeConfig, config["osc"]["message"].value_or("/test"));
    activeConfig.enabled = config["osc"]["enabled"].value_or(true);
}

void OSCManager::parseCommand(OSCConfig& config, const std::string& rawLine) {
    config.osc_address.clear();
    config.args.clear();

    std::stringstream ss(rawLine);
    std::string token;
    bool isFirstToken = true;

    while (ss >> token) {
        // First token is always the address path
        if(isFirstToken) {
            config.osc_address = token;
            isFirstToken = false;
            continue;
        }

        // Smart-type matching loop for arguments
        if(token == "true" || token == "TRUE") {
            config.args.push_back(true);
        } else if(token == "false" || token == "FALSE") {
            config.args.push_back(false);
        } else if(isInt(token)) {
            config.args.push_back(std::stoi(token));
        } else if(isFloat(token)) {
            config.args.push_back(std::stof(token));
        } else {
            config.args.push_back(token);
        }
    }
}

OSCMessage OSCManager::compileMessage(OSCMessage& msg) {
    msg.setAddress(activeConfig.osc_address.c_str());
    for(const auto& arg : activeConfig.args) {
        if(std::holds_alternative<int>(arg)) {
            msg.add(std::get<int>(arg));
        } else if(std::holds_alternative<float>(arg)) {
            msg.add(std::get<float>(arg));
        } else if(std::holds_alternative<bool>(arg)) {
            msg.add(std::get<bool>(arg));
        } else {
            msg.add(std::get<std::string>(arg).c_str());
        }
    }
    return msg;
}

const std::string& OSCManager::getMessage() const {
    std::string msg = getOSCAddress();

    for(const auto& arg : activeConfig.args) {
        if(std::holds_alternative<int>(arg)) {
            msg += std::to_string(std::get<int>(arg));
        } else if(std::holds_alternative<float>(arg)) {
            msg += std::to_string(std::get<float>(arg));
        } else if(std::holds_alternative<bool>(arg)) {
            msg += std::get<bool>(arg) ? "true" : "false";
        } else {
            msg += std::get<std::string>(arg).c_str();
        }
    }

    return msg;
}
