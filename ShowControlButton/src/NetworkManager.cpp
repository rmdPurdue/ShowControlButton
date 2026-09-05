#include "NetworkManager.h"

bool NetworkManager::connectHardware() {
    Serial.println("[NetworkManager] Attempting hardware initialization...");

    if(activeConfig.dhcpEnabled) {
        if(Ethernet.begin(activeConfig.mac_address) == 1) {
            Serial.print("[NetworkManager] DHCP Success! IP: ");
            Serial.println(Ethernet.localIP());
            isConnected = true;
            return true;
        }
        Serial.println("[NetworkManager] DHCP failed.");
        isConnected = false;
        return false;
    } else {
        IPAddress ip, subnet, gw;
        if(ip.fromString(activeConfig.ipAddress.c_str()) &&
           subnet.fromString(activeConfig.subnet.c_str()) &&
           gw.fromString(activeConfig.gateway.c_str())) {

            Ethernet.begin(activeConfig.mac_address, ip, gw, gw, subnet);
            Serial.print("[NetworkManager] Static IP applied: ");
            Serial.println(Ethernet.localIP());
            isConnected = true;
            return true;
        }
        Serial.println("[NetworkManager] Static IP Parsing failed.");
        isConnected = false;
        return false;
    }
}

void NetworkManager::parseMacAddress(const std::string& macStr, uint8_t* macBytes) {
    int values[6];
    if(sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
              &values[0], &values[1], &values[2],
              &values[3], &values[4], &values[5]) == 6) {
        for(int i =0; i < 6; ++i) {
            macBytes[i] = (uint8_t)values[i];
        }
    } else {
        uint8_t dummy[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
        memcpy(macBytes, dummy, 6);
    }
}

void NetworkManager::init(const NetworkConfig& config, uint8_t pin) {
    activeConfig = config;
    Ethernet.init(pin);

    lastLinkState = Ethernet.linkStatus();
    if(lastLinkState == LinkON) {
        connectHardware();
    } else {
        Serial.println("[NetworkManager] Boot Warning: no ethernet cable detected.");
        isConnected = false;
    }
}

void NetworkManager::updateConnectionStatus() {
    if(millis() - lastCheckTime < checkInterval) {
        return;
    }
    lastCheckTime = millis();

    EthernetLinkStatus currentState = Ethernet.linkStatus();

    if(currentState != lastLinkState) {
        if(currentState == LinkON) {
            Serial.println("[NetworkManager] Cable plugged in. Re-initializing link.");
            connectHardware();
        } else if(currentState == LinkOFF) {
            Serial.println("[NetworkManager] Cable unplugged! Network connection lost.");
            isConnected = false;
        }
        lastLinkState = currentState;
    }

    if(isConnected && activeConfig.dhcpEnabled) {
        Ethernet.maintain();
    }
}

void NetworkManager::updateConfiguration(const NetworkConfig& newConfig) {
    activeConfig = newConfig;
}

NetworkConfig NetworkManager::getConfiguration() const {
    return activeConfig;
}

bool NetworkManager::loadConfigFromToml(const std::string& tomlString) {
    toml::parse_result result = toml::parse(tomlString);
    if(!result) return false; // Parse error

    const toml::table& config = result.table();
    std::string rawMac = config["ethernet"]["mac"].value_or("DE:AD:BE:EF:FE:ED");
    parseMacAddress(rawMac, activeConfig.mac_address);
    activeConfig.ipAddress = config["ethernet"]["ip"].value_or("192.168.1.10");
    activeConfig.subnet = config["ethernet"]["subnet"].value_or("255.255.254.0");
    activeConfig.gateway = config["ethernet"]["gateway"].value_or("192.168.1.1");
    activeConfig.dhcpEnabled = config["ethernet"]["dhcp_en"].value_or(false);
}

bool NetworkManager::isNetworkReady() const {
    return isConnected && (Ethernet.linkStatus() == LinkON);
}
