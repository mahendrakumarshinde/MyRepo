#include "IUWiFiManager.h"


/* =============================================================================
    Core
============================================================================= */

IUWiFiManager::IUWiFiManager(uint32_t connectTimeout, uint32_t timeout,
                             int minSignalQuality) :
    m_minSignalQuality(minSignalQuality),
    m_connectTimeout(connectTimeout),
    m_timeout(timeout),
    m_lastReconnectionAttempt(0),
    m_lastSentHeartbeat(0),
    m_lastConnected(0),
    m_newCredentialStart(0)
{
    strcpy(m_staticIp, "");
    strcpy(m_staticGateway, "");
    strcpy(m_staticSubnet, "");
    resetUserSSID();
    resetUserPassword();
}

/**
 * 
 */
bool IUWiFiManager::hasTimedOut(bool resetTimer)
{
    uint32_t current = millis();
    if (resetTimer)
    {
         m_lastConnected = current;
         return false;
    }
    else if (m_timeout == 0) // No timeout
    {
        return false;
    }
    else if (current > m_lastConnected + m_timeout ||
             m_lastConnected > current)  // Handle millis overflow
    {
        return true;
    }
    return false;
}


/* =============================================================================
    User set SSID and password
============================================================================= */

/**
 * 
 */
void IUWiFiManager::resetUserSSID()
{
    m_newSSID = false;
    for (uint8_t i = 0; i < wifiCredentialLength; ++i)
    {
        m_userSSID[i] = '\0';
    }
}

/**
 * 
 */
void IUWiFiManager::resetUserPassword()
{
    m_newPassword = false;
    for (uint8_t i = 0; i < wifiCredentialLength; ++i)
    {
        m_userPassword[i] = '\0';
    }
}

/**
 * 
 */
void IUWiFiManager::saveCurrentCredentials()
{
    if (WiFi.SSID().length() == 0)
    {
        if (debugMode)
        {
            debugPrint("No current credentials: "
                       "keeping whatever is already saved");
        }
        return;
    }
    resetUserSSID();
    resetUserPassword();
    strcpy(m_userSSID, WiFi.SSID().c_str());
    strcpy(m_userPassword, WiFi.psk().c_str());
    m_newSSID = true;
    m_newPassword = true;
}

/**
 * 
 */
void IUWiFiManager::addUserSSID(char *newSSID, uint8_t length)
{
    strncpy(m_userSSID, newSSID, length);
    if (debugMode)
    {
        debugPrint("New SSID:", false);
        debugPrint(m_userSSID);
    }
    m_newSSID = true;
    if (haveCredentialsTimeout())
    {
        resetUserPassword();
        m_newCredentialStart = millis();
    }
    else
    {
        if (m_newSSID && m_newPassword)
        {
            reconnect(true);
        }
    }
}

/**
 * 
 */
void IUWiFiManager::addUserPassword(char *newPassword, uint8_t length)
{
    strncpy(m_userPassword, newPassword, length);
    if (debugMode)
    {
        debugPrint("New pw:", false);
        debugPrint(m_userPassword);
    }
    m_newPassword = true;
    if (haveCredentialsTimeout())
    {
        resetUserSSID();
        m_newCredentialStart = millis();
    }
    else
    {
        if (m_newSSID && m_newPassword)
        {
            reconnect(true);
        }
    }
}

/**
 * 
 */
bool IUWiFiManager::haveCredentialsTimeout()
{
    uint32_t now = millis();
    bool result =  !(m_newCredentialStart > now ||
                     m_newCredentialStart + newWiFiCredentialsTimeout > now);
    if (debugMode)
    {
        debugPrint("Credential timeout: ", false);
        debugPrint(result);
    }
    return result;
}

/*
 * 
 */
bool IUWiFiManager::hasSavedCredentials()
{
    if (m_newSSID && m_newPassword)
    {
        return true;
    }
    if (WiFi.SSID().length() > 0)
    {
        return true;
    }
    return false;
}


/* =============================================================================
    Wifi connection and status
============================================================================= */

/**
 * Check if the WiFi is connected and if not, attempt to reconnect.
 */
bool IUWiFiManager::reconnect(bool forceNewCredentials)
{
    uint32_t current = millis();
    String currentSSID = WiFi.SSID();
    if (forceNewCredentials &&
        !(currentSSID.length() > 0 &&
          strcmp(currentSSID.c_str(), m_userSSID) == 0))
    {
        if (debugMode)
        {
            debugPrint("Forcing WiFi reconnection with new credentials and"
                       " resetting main timeout");
        }
        m_lastConnected = current;
        if (WiFi.isConnected())
        {
            WiFi.disconnect(true);
        }
        if (!(m_newSSID && m_newPassword))
        {
            if (debugMode)
            {
                debugPrint("No new SSID or passsword (may have timed out");
            }
            return false;
        }
        if (debugMode)
        {
            debugPrint("Attempting connection with new credentials");
            debugPrint(m_userSSID);
            debugPrint(m_userPassword);
        }
        WiFi.mode(WIFI_STA);
        WiFi.begin(m_userSSID, m_userPassword);
        m_lastReconnectionAttempt = 0;
    }
    else if (WiFi.isConnected())
    {
        // Already connected with the right credentials
        m_lastReconnectionAttempt = 0;
        return true;
    }
    else
    {
        // No new credentials, but disconnected
        if (current > m_lastReconnectionAttempt + reconnectionInterval ||
            m_lastReconnectionAttempt > current)  // Handle millis overflow
        {
            if (debugMode)
            {
                debugPrint(current, false);
                debugPrint(": WiFi is disconnected, attempting reconnection");
            }
            m_lastReconnectionAttempt = current;
            WiFi.mode(WIFI_STA);
            if (WiFi.SSID().length() > 0)
            {
                WiFi.begin();
            }
            else if (m_newSSID && m_newPassword)
            {
                WiFi.begin(m_userSSID, m_userPassword);
            }
            else
            {
                if (debugMode)
                {
                    debugPrint("There are no saved credentials!");
                }
                return false;
            }
        }
    }
    bool connectSuccess = (waitForConnectResult() == WL_CONNECTED);
    if (connectSuccess && debugMode)
    {
        debugPrint("Connected to ", false);
        debugPrint(WiFi.SSID());
    }
    return connectSuccess;
}

/**
 * 
 */
uint8_t IUWiFiManager::waitForConnectResult()
{
    if (m_connectTimeout == 0)
    {
        return WiFi.waitForConnectResult();
    }
    else
    {
        uint32_t startT = millis();
        bool keepConnecting = true;
        uint8_t status;
        while (keepConnecting)
        {
            status = WiFi.status();
            if (millis() > startT + m_connectTimeout)
            {
                keepConnecting = false;
            }
            if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
            {
                keepConnecting = false;
            }
            delay(100);
        }
        return status;
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
 *  250 char long.
 */
void IUWiFiManager::getWifiInfo(char *destination, bool mqttOn)
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
    strcat(destination, "\",\"MQTT\":\"");
    if (mqttOn)
    {
        strcat(destination, "on");
    } else {
        strcat(destination, "off");
    }
    strcat(destination, "\"}");
}

/**
 * 
 */
void IUWiFiManager::debugPrintWifiInfo()
{
    #ifdef DEBUGMODE
    WiFiMode_t currMode = WiFi.getMode();
    debugPrint("WiFi mode: ", false);
    debugPrint((uint8_t) currMode);
    if (currMode == WIFI_STA)
    {
        debugPrint("WiFi SSID: ", false);
        debugPrint(WiFi.SSID());
        debugPrint("WiFi psk: ", false);
        debugPrint(WiFi.psk());
        debugPrint("WiFi RSSI: ", false);
        debugPrint(WiFi.RSSI());
    }
    debugPrint("User SSID: ", false);
    if (m_newSSID)
    {
        debugPrint(m_userSSID);
    } else {
        debugPrint("<none>");
    }
    debugPrint("User psk: ", false);
    if (m_userPassword)
    {
        debugPrint(m_userPassword);
    } else {
        debugPrint("<none>");
    }
    #endif
}


/* =============================================================================
    Instanciation
============================================================================= */

IUWiFiManager iuWifiManager(1000, 20000, 8);
