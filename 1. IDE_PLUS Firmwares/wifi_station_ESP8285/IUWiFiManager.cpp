#include "IUWiFiManager.h"


/* =============================================================================
    Core
============================================================================= */

IUWiFiManager::IUWiFiManager(uint32_t timeout, int minSignalQuality) :
    m_timeout(timeout),
    m_minSignalQuality(minSignalQuality),
    m_lastReconnectionAttempt(0),
    m_lastSentHeartbeat(0)
{
    strcpy(m_staticIp, "");
    strcpy(m_staticGateway, "");
    strcpy(m_staticSubnet, "");
}

/**
 * Try to connect from saved credentials or ask for new ones and then connect.
 *
 * The WiFi manager will try to connect to a saved WiFi network, and if it
 * fails, will switch the ESP to AP mode and launch web portal to set up
 * targe tWiFi SSID and password.
 */
void IUWiFiManager::manageWifi(const char *apName, const char *apPassword,
                               void (*onConnectionCallback)())
{
    WiFiManager wifiManager;
    wifiManager.setDebugOutput(debugMode);

    // TODO Finish the static IP setup
    // WiFiManagerParameter staticIpParam(
    //     "Static IP (only fill if a Static Ip is needed)", "0.0.0.0",
    //     staticIp, 16);
    // WiFiManagerParameter staticGatewayParam("Gateway", "0.0.0.0",
    //                                         staticGateway, 16);
    // WiFiManagerParameter staticSubnetParam("Subnet", "0.0.0.0",
    //                                        staticSubnet, 16);
    // wifiManager.addParameter(&staticIpParam);
    // wifiManager.addParameter(&staticGatewayParam);
    // wifiManager.addParameter(&staticSubnetParam);

    //set minimu quality of signal so it ignores AP's under that quality
    //defaults to 8%
    wifiManager.setMinimumSignalQuality(m_minSignalQuality);

    //reset settings - for testing
    //wifiManager.resetSettings();
    wifiManager.setAPCallback(webPortalStartedCallback);
    wifiManager.setTimeout(m_timeout);

    //fetches ssid and pass and tries to connect
    // Blocking loop awaiting configuration
    bool success;
    if (apName)
    {
        success = wifiManager.autoConnect(apName, apPassword);
    }
    else
    {
        success = wifiManager.autoConnect();
    }
    if(!success)
    {
        if (debugMode)
        {
            debugPrint("Failed to connect and hit timeout => resetting");
        }
        ESP.reset();
    }
    onConnectionCallback();
    if (debugMode)
    {
        debugPrint("Connected to ", false);
        debugPrint(WiFi.SSID());
    }
}

/**
 * Check if the WiFi is connected and if not, attempt to reconnect.
 */
bool IUWiFiManager::reconnect(const char *apName, const char *apPassword,
                              void (*onConnectionCallback)())
{
    uint32_t current = millis();
    // Reconnection if needed
    if (WiFi.isConnected())
    {
        m_lastReconnectionAttempt = 0;
        return true;
    }
    else
    {
        if (current > m_lastReconnectionAttempt + reconnectionInterval ||
            m_lastReconnectionAttempt > current)  // Handle millis overflow
        {
            manageWifi(apName, apPassword, onConnectionCallback);
            m_lastReconnectionAttempt = millis();
        }
        return WiFi.isConnected();
    }
}

/**
 *
 */
bool IUWiFiManager::isTimeToSendWifiInfo()
{
    uint32_t current = millis();
    if (WiFi.isConnected())
    {
        if (m_lastSentHeartbeat == 0 ||
            current > m_lastSentHeartbeat + heartbeatInterval ||
            m_lastSentHeartbeat > current)  // Handle millis overflow
        {
            m_lastSentHeartbeat = current;
            return true;
        }
    }
    return false;
}





/* =============================================================================
    Static IP settings
============================================================================= */

/**
 *
 */
void IUWiFiManager::setStaticIpConfig(char *staticIp, char *staticGateway,
                                      char *staticSubnet)
{
    // TODO Implement
}


/* =============================================================================
    Debugging
============================================================================= */

/**
 * Format and output wifi info into a JSON-parsable char array.
 *
 * @param destination A char array to hold the function output. Must be at least
 *  200 char long.
 */
void IUWiFiManager::getWifiInfo(char *destination)
{
    strcpy(destination, "{\"ssid\":\"");
    strcat(destination, WiFi.SSID().c_str());
    strcat(destination, "\",\"rssi\":");
    strcat(destination, String(WiFi.RSSI()).c_str());
    strcat(destination, ",\"local_ip\":\"");
    strcat(destination, WiFi.localIP().toString().c_str());
    strcat(destination, "\",\"subnet\":\"");
    strcat(destination, WiFi.subnetMask().toString().c_str());
    strcat(destination, "\",\"gateway\":\"");
    strcat(destination, WiFi.gatewayIP().toString().c_str());
    strcat(destination, "\",\"dns\":\"");
    strcat(destination, WiFi.dnsIP().toString().c_str());
    strcat(destination, "\",\"wifi_mac\":\"");
    strcat(destination, WiFi.macAddress().c_str());
    strcat(destination, "\"}");
}


/* =============================================================================
    Instanciation
============================================================================= */

IUWiFiManager iuWifiManager(180, 8);
