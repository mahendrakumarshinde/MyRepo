#include "IUWiFiManager.h"


/* =============================================================================
    Core
============================================================================= */

IUWiFiManager::IUWiFiManager(uint32_t timeout, int minSignalQuality) :
    m_timeout(timeout),
    m_minSignalQuality(minSignalQuality)
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
void IUWiFiManager::manageWifi(char const *apName, char const *apPassword,
                               bool printInfo)
{
    WiFiManager wifiManager;

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
    if (printInfo)
    {
        wifiManager.setAPCallback(webPortalStartedCallback);
    }
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
        delay(1000);
    }
    if (debugMode)
    {
        debugPrint("Connected to ", false);
        debugPrint(WiFi.SSID());
    }
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
    Core
============================================================================= */

/**
 *
 */
void IUWiFiManager::printWifiInfo()
{
    #ifdef DEBUGMODE
    debugPrint("WiFi mac: ", false);
    debugPrint(WiFi.macAddress());
    debugPrint("SSID: ", false);
    debugPrint(WiFi.SSID());
    debugPrint("Status: ", false);
    if (!WiFi.isConnected())
    {
        debugPrint("dis", false);
    }
    debugPrint("connected");
    debugPrint("signal strength (RSSI):", false);
    debugPrint(WiFi.RSSI(), false);
    debugPrint(" dBm");
    debugPrint("Local IP: ", false);
    debugPrint(WiFi.localIP());
    debugPrint("Subnet Mask: ", false);
    debugPrint(WiFi.subnetMask());
    debugPrint("Gateway IP: ", false);
    debugPrint(WiFi.gatewayIP());
    debugPrint("DNS IP: ", false);
    debugPrint(WiFi.dnsIP());
    #endif
}


/* =============================================================================
    Instanciation
============================================================================= */

IUWiFiManager iuWifiManager(180, 8);
