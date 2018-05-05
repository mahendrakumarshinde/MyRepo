#include "IUESP8285.h"

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                     uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                     uint32_t rate, char stopChar,
                     uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate, stopChar,
             dataReceptionTimeout)
{
    m_credentialValidator.setTimeout(wifiConfigReceptionTimeout);
    m_staticConfigValidator.setTimeout(wifiConfigReceptionTimeout);
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUESP8285::setupHardware()
{
    // Check if UART is being driven by something else (eg: FTDI connnector)
    // used to flash the ESP8285
    // TODO Test the following
//    pinMode(UART_TX_PIN, INPUT_PULLDOWN);
//    delay(100);
//    if(digitalRead(UART_TX_PIN))
//    {
//        // UART driven by something else
//        // TODO How does this behave when there is a battery?
//        IUSerial::suspend();
//        return;
//    }
    begin();
    hardReset();
    setPowerMode(PowerMode::REGULAR);
}

/**
 * Manage component power modes
 */
void IUESP8285::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    manageAutoSleep();
}


/* =============================================================================
    WiFi Maintenance functions
============================================================================= */

/**
 * Send commands to WiFi chip to manage its sleeping cycle.
 *
 * When not connected, WiFi cycle through connection attempts / sleeping phase.
 * When connected, WiFi use light-sleep mode to maintain connection to AP at a
 * lower energy cost.
 */
void IUESP8285::manageAutoSleep()
{
    uint32_t now = millis();
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
            sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
            break;
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            if (m_connected)
            {
                m_wakeUpNow = false;
                sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                m_awakeTimerStart = now;
            }
            else if (m_wakeUpNow) // Wake up now and stay awake
            {
                sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                sendWiFiCredentials();
                m_awakeTimerStart = now;  // Reset auto-sleep start timer
            }
            else if (m_sleeping)  // Not connected, already sleeping
            {
                if (now - m_sleepTimerStart > m_autoSleepDuration)
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                    sendWiFiCredentials();
                    m_awakeTimerStart = now;  // Reset auto-sleep start timer
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                }
            }
            else  // Not connected and not sleeping
            {
                m_wakeUpNow = false;
                if (now - m_awakeTimerStart > m_autoSleepDelay)
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                    m_sleepTimerStart = now;  // Reset auto-sleep start timer
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                }
            }
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            // TODO Implement sleep duration
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
    }
    if ((uint8_t) m_powerMode > (uint8_t) PowerMode::SLEEP && !m_sleeping)
    {
        sendWiFiCredentials();
        if ((uint64_t) m_macAddress == 0)
        {
            sendMSPCommand(MSPCommand::ASK_WIFI_MAC);
        }
    }
}


/* =============================================================================
    Local storage (flash) management
============================================================================= */

bool IUESP8285::loadConfigFromFlash(IUFlash *iuFlashPtr,
                                    IUFlash::storedConfig configType)
{
    char strConfig[256];
    size_t charCount =  iuFlashPtr->readConfig(configType, strConfig, 256);
    if (charCount == 0)
    {
        if (debugMode)
        {
            debugPrint("No WiFi config found in Flash");
        }
        return false;
    }
    JsonObject& config = m_jsonBuffer.parseObject(strConfig);
    if (!config.success())
    {
        if (debugMode)
        {
            debugPrint("Couln't parse WiFi config found in flash");
        }
        return false;
    }
    bool success = true;  // success if at least "ssid" and "psk" found
    const char *value;
    value = config.get<const char*>("ssid");
    if (value) { setSSID(value, strlen(value)); }
    else { success = false; }
    value = config.get<const char*>("psk");
    if (value) { setPassword(value, strlen(value)); }
    else { success = false; }
    value = config.get<const char*>("static");
    if (value) { setStaticIP(value, strlen(value)); }
    value = config.get<const char*>("gateway");
    if (value) { setGateway(value, strlen(value)); }
    value = config.get<const char*>("subnet");
    if (value) { setSubnetMask(value, strlen(value)); }
    // Send to WiFi module (consistency check is done inside send functions)
    sendWiFiCredentials();
    sendStaticConfig();
    return success;
}

/**
 *
 */
void IUESP8285::saveConfigToFlash(IUFlash *iuFlashPtr,
                                  IUFlash::storedConfig configType)
{
    if (!(iuFlashPtr && iuFlashPtr->available()))
    {
        return;
    }
    if (m_credentialValidator.completed())
    {
        if (m_staticConfigValidator.completed())
        {
            size_t configlen =  2 * (size_t) wifiCredentialLength +
                                15 * 3 +  // space for IP address
                                57;  // JSON keys and markers
            char config[configlen];
            snprintf(
                config, configlen, "{\"ssid\":\"%s\",\"psk\":\"%s\","
                "\"static\":\"%u.%u.%u.%u\",\"gateway\":\"%u.%u.%u.%u\","
                "\"subnet\":\"%u.%u.%u.%u\"}", m_ssid, m_psk,
                m_staticIP[0], m_staticIP[1], m_staticIP[2], m_staticIP[3],
                m_staticGateway[0], m_staticGateway[1], m_staticGateway[2],
                m_staticGateway[3], m_staticSubnet[0], m_staticSubnet[1],
                m_staticSubnet[2], m_staticSubnet[3]);
            iuFlashPtr->writeConfig(configType, config);

        }
        else
        {
            size_t configlen =  2 * (size_t) wifiCredentialLength + 21;
            char config[configlen];
            snprintf(config, configlen, "{\"ssid\":\"%s\",\"psk\":\"%s\"}",
                     m_ssid, m_psk);
            iuFlashPtr->writeConfig(configType, config);
        }
    }
}


/* =============================================================================
    Network Configuration
============================================================================= */

void IUESP8285::setSSID(const char *ssid, uint8_t length)
{
    if (m_credentialValidator.hasTimedOut())
    {
        m_credentialValidator.reset();
    }
    uint8_t charCount = min(wifiCredentialLength, length);
    strncpy(m_ssid, ssid, charCount);
    for (uint8_t i = charCount; i < wifiCredentialLength; ++i)
    {
        m_ssid[i] = 0;
    }
    m_credentialValidator.receivedMessage(0);
}

void IUESP8285::setPassword(const char *psk, uint8_t length)
{
    if (m_credentialValidator.hasTimedOut())
    {
        m_credentialValidator.reset();
    }
    uint8_t charCount = min(wifiCredentialLength, length);
    strncpy(m_psk, psk, charCount);
    for (uint8_t i = charCount; i < wifiCredentialLength; ++i)
    {
        m_psk[i] = 0;
    }
    m_credentialValidator.receivedMessage(1);
}

void IUESP8285::setStaticIP(IPAddress staticIP)
{
    if (m_staticConfigValidator.hasTimedOut())
    {
        m_staticConfigValidator.reset();
    }
    m_staticIP = staticIP;
    m_staticConfigValidator.receivedMessage(0);
}

bool IUESP8285::setStaticIP(const char *staticIP, uint8_t len)
{
    IPAddress tempAddress;
    char tempArr[len];
    for (uint8_t i = 0; i < len; ++i)
    {
        tempArr[i] = staticIP[i];
    }
    // Make sure the sub string is null terminated.
    tempArr[len - 1] = 0;
    bool success = tempAddress.fromString(tempArr);
    if (success)
    {
        setStaticIP(tempAddress);
    }
    return success;
}

void IUESP8285::setGateway(IPAddress gatewayIP)
{
    if (m_staticConfigValidator.hasTimedOut())
    {
        m_staticConfigValidator.reset();
    }
    m_staticGateway = gatewayIP;
    m_staticConfigValidator.receivedMessage(1);
}

bool IUESP8285::setGateway(const char *gatewayIP, uint8_t len)
{
    IPAddress tempAddress;
    char tempArr[len];
    for (uint8_t i = 0; i < len; ++i)
    {
        tempArr[i] = gatewayIP[i];
    }
    // Make sure the sub string is null terminated.
    tempArr[len - 1] = 0;
    bool success = tempAddress.fromString(tempArr);
    if (success)
    {
        setGateway(tempAddress);
    }
    return success;
}

void IUESP8285::setSubnetMask(IPAddress subnetIP)
{
    if (m_staticConfigValidator.hasTimedOut())
    {
        m_staticConfigValidator.reset();
    }
    m_staticSubnet = subnetIP;
    m_staticConfigValidator.receivedMessage(2);
}

bool IUESP8285::setSubnetMask(const char *subnetIP, uint8_t len)
{
    IPAddress tempAddress;
    char tempArr[len];
    for (uint8_t i = 0; i < len; ++i)
    {
        tempArr[i] = subnetIP[i];
    }
    // Make sure the sub string is null terminated.
    tempArr[len - 1] = 0;
    bool success = tempAddress.fromString(tempArr);
    if (success)
    {
        setSubnetMask(tempAddress);
    }
    return success;
}


/* =============================================================================
    User Inbound communication
============================================================================= */

/**
 *
 */
void IUESP8285::processUserMessage(char *buff, IUFlash *iuFlashPtr)
{
    if (strncmp(buff, "WIFI-HARDRESET", 15) == 0)
    {
        hardReset();
    }
    else if (strncmp(buff, "WIFI-USE-SAVED", 15) == 0)
    {
        if (debugMode)
        {
            debugPrint("Current WiFi info: ", false);
            debugPrint(m_ssid, false);
            debugPrint(", ", false);
            debugPrint(m_psk);
        }
        connect();
    }
    else if (strncmp(buff, "WIFI-SSID-", 10) == 0)
    {
        uint16_t len = strlen(buff);
        if (strcmp(&buff[len - 10], "-DISS-IFIW") != 0)
        {
            if (debugMode)
            {
                debugPrint("Unparsable SSID");
            }
            return;
        }
        setSSID(&buff[10], len - 20);
        sendWiFiCredentials();
        saveConfigToFlash(iuFlashPtr);
    }
    else if (strncmp(buff, "WIFI-PW-", 8) == 0)
    {
        uint16_t len = strlen(buff);
        if (strcmp(&buff[len - 8], "-WP-IFIW") != 0)
        {
            if (debugMode)
            {
                debugPrint("Unparsable password");
            }
            return;
        }
        setPassword(&buff[8], len - 16);
        sendWiFiCredentials();
        saveConfigToFlash(iuFlashPtr);
    }
    else if (strncmp(buff, "WIFI-IP-", 8) == 0)
    {
        uint16_t len = strlen(buff);
        if (strcmp(&buff[len - 8], "-PI-IFIW") != 0 ||
            !setStaticIP(&buff[8], len - 16))
        {
            if (debugMode)
            {
                debugPrint("Unparsable Static IP");
            }
            return;
        }
        sendStaticConfig();
        saveConfigToFlash(iuFlashPtr);
    }
    else if (strncmp(buff, "WIFI-GTW-", 9) == 0)
    {
        uint16_t len = strlen(buff);
        if (strcmp(&buff[len - 9], "-WTG-IFIW") != 0 ||
            !setGateway(&buff[9], len - 18))
        {
            if (debugMode)
            {
                debugPrint("Unparsable Gateway IP");
            }
            return;
        }
        sendStaticConfig();
        saveConfigToFlash(iuFlashPtr);
    }
    else if (strncmp(buff, "WIFI-SM-", 8) == 0)
    {
        uint16_t len = strlen(buff);
        if (strcmp(&buff[len - 8], "-MS-IFIW") != 0 ||
            !setSubnetMask(&buff[8], len - 16))
        {
            if (debugMode)
            {
                debugPrint("Unparsable Subnet Mask IP");
            }
            return;
        }
        sendStaticConfig();
        saveConfigToFlash(iuFlashPtr);
    }
}


/* =============================================================================
    Guest Inbound communication
============================================================================= */

/**
 *
 */
bool IUESP8285::processChipMessage()
{
    bool commandFound = true;
    switch (m_mspCommand)
    {
        case MSPCommand::RECEIVE_WIFI_MAC:
            m_macAddress = mspReadMacAddress();
            if (loopDebugMode)
            {
                debugPrint("RECEIVE_WIFI_MAC: ", false);
                debugPrint(m_macAddress);
            }
            break;
        case MSPCommand::WIFI_CONFIRM_NEW_CREDENTIALS:
            if (loopDebugMode)
            {
                debugPrint("WIFI_CONFIRM_NEW_CREDENTIALS: ", false);
                debugPrint(m_buffer);
            }
            m_credentialReceptionConfirmed = true;
            break;
        case MSPCommand::WIFI_CONFIRM_NEW_STATIC_CONFIG:
            if (loopDebugMode)
            {
                debugPrint("WIFI_CONFIRM_NEW_STATIC_CONFIG: ", false);
                debugPrint(m_buffer);
            }
            m_staticConfigReceptionConfirmed = true;
            break;
        case MSPCommand::WIFI_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_CONNECTED"); }
            m_connected = true;
            m_working = false;
            m_awakeTimerStart = millis();
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_DISCONNECTED"); }
            m_connected = false;
            if (millis() - m_displayConnectAttemptStart >
                displayConnectAttemptTimeout)
            {
                m_working = false;
                m_displayConnectAttemptStart = 0;
            }
            break;
        case MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS:
            if (loopDebugMode)
            {
                debugPrint("WIFI_ALERT_NO_SAVED_CREDENTIALS");
            }
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_SLEEPING:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_SLEEPING"); }
            if (!m_sleeping)
            {
                m_sleepTimerStart = millis();
            }
            m_sleeping = true;
            break;
        case MSPCommand::WIFI_ALERT_AWAKE:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_AWAKE"); }
            if (m_sleeping)
            {
                m_awakeTimerStart = millis();
            }
            m_sleeping = false;
            break;
        case MSPCommand::WIFI_REQUEST_ACTION:
            if (loopDebugMode) { debugPrint("WIFI_REQUEST_ACTION"); }
            manageAutoSleep();
            break;
        default:
            commandFound = false;
            break;
    }
    return commandFound;
}

/* =============================================================================
    Guest Outbound communication
============================================================================= */

/**
 *
 */
void IUESP8285::sendWiFiCredentials()
{
    if (m_credentialValidator.completed())
    {
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_SSID, m_ssid);
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_PASSWORD, m_psk);
        m_credentialSent = true;
        m_credentialReceptionConfirmed = false;
        m_displayConnectAttemptStart = millis();
        m_working = true;
    }
}

/**
 *
 */
void IUESP8285::forgetCredentials()
{
    sendMSPCommand(MSPCommand::WIFI_FORGET_CREDENTIALS);
    m_working = true;
}

/**
 *
 */
void IUESP8285::sendStaticConfig()
{
    if (m_staticConfigValidator.completed())
    {
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_STATIC_IP, m_staticIP);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_GATEWAY, m_staticGateway);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_SUBNET, m_staticSubnet);
        m_staticConfigSent = true;
        m_staticConfigReceptionConfirmed = false;
        m_displayConnectAttemptStart = millis();
        m_working = true;
    }
}

/**
 *
 */
void IUESP8285::forgetStaticConfig()
{
    sendMSPCommand(MSPCommand::WIFI_FORGET_STATIC_CONFIG);
    m_working = true;
}

/**
 *
 */
void IUESP8285::connect()
{
    sendMSPCommand(MSPCommand::WIFI_CONNECT);
    m_displayConnectAttemptStart = millis();
    m_working = true;
}

/**
 *
 */
void IUESP8285::disconnect()
{
    sendMSPCommand(MSPCommand::WIFI_DISCONNECT);
    m_working = true;
}
