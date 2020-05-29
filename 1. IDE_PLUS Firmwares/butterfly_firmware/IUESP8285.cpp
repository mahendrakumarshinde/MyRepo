#include "IUESP8285.h"
#include "RawDataState.h"

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

void IUESP8285::m_setConnectedStatus(bool status)
{
    if (!m_connected && status) {
        // Reset last publication confirmation timer on establishing connection.
        m_lastConfirmedPublication = millis();
    }
    bool useCallbacks = (m_connected != status);
    m_connected = status;
    if (useCallbacks) {
        if (m_connected) {
            if (m_onConnect) {
                m_onConnect();
            }
        } else {
            if (m_onDisconnect) {
                m_onDisconnect();
            }
        }
    }
}

bool IUESP8285::arePublicationsFailing()
{
    if (!m_connected) {
        return false;  // No connection, so no publication
    }
    return (millis() - m_lastConfirmedPublication > confirmPublicationTimeout);
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
    // IDE1.5_PORT_CHANGE - Dont send WiFi Credentials (hardReset->turnOn->sendWiFiCredentials()) 
    // as MQTT resets ESP during MQTT Config.
    m_credentialSent = true;
    pinMode(ESP32_ENABLE_PIN, OUTPUT);
    hardReset();
    begin();
    setPowerMode(PowerMode::REGULAR);
    m_credentialSent = false;
}

/**
 * Disable the ESP8285
 */
void IUESP8285::turnOn(bool forceTimerReset)
{
    if (!m_on)
    {
        digitalWrite(ESP32_ENABLE_PIN, HIGH);
        if(forceTimerReset == true) {
            m_credentialSent = false;
            delay(6000); // IDE1.5_PORT_CHANGE -- After ESP Reset, wait for ESP32 to boot up
        }
        m_on = true;
        m_awakeTimerStart = millis();
        sendWiFiCredentials();
        if (loopDebugMode)
        {
            debugPrint("Wifi turned on");
        }
    }
    else if (forceTimerReset)
    {
        m_awakeTimerStart = millis();
    }
}

/**
 * Disable the ESP8285
 */
void IUESP8285::turnOff()
{
    m_setConnectedStatus(false);
    if (m_on)
    {
        digitalWrite(ESP32_ENABLE_PIN, LOW);
        m_on = false;
        m_sleepTimerStart = millis();  // Reset auto-sleep start timer
        if (loopDebugMode)
        {
            debugPrint("Wifi turned off");
        }
        // IDE1.5_PORT_CHANGE - To reset Raw data transmission on ESP reset
        RawDataState::startRawDataCollection = false;
        RawDataState::rawDataTransmissionInProgress = false;
    }
}

/**
 * Hardware reset by disabling then re-enabling the ESP8285
 */
void IUESP8285::hardReset()
{
    turnOff();
    delay(100);
    resetBuffer();
    turnOn();
}

/**
 * Manage component power modes
 */
void IUESP8285::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    // When we change the power mode, we expect the WiFi to wake up instantly.
    manageAutoSleep(m_powerMode > PowerMode::SLEEP);
}

/**
 * Send commands to WiFi chip to manage its sleeping cycle.
 *
 * When not connected, WiFi cycle through connection attempts / sleeping phase.
 * When connected, WiFi use light-sleep mode to maintain connection to AP at a
 * lower energy cost.
 */
void IUESP8285::manageAutoSleep(bool wakeUpNow)
{
    uint32_t now = millis();
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
            turnOn(true);
            break;
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            if (m_connected)
            {
                turnOn();
            }
            else if (wakeUpNow)
            {
                turnOn(true);
            }
            // Not connected, sleeping but need to wake up
            else if (!m_on && now - m_sleepTimerStart > m_autoSleepDuration)
            {
                turnOn();
                debugPrint("Slept for ", false);
                debugPrint(now - m_sleepTimerStart, false);
                debugPrint("ms");
            }
            // Not connected, not sleeping yet, but need to go to sleep
            else if (m_on && now - m_awakeTimerStart > m_autoSleepDelay)
            {
                turnOff();
                if (loopDebugMode)
                {
                    debugPrint("Reason: exceeded disconnection timeout (", false);
                    debugPrint(m_autoSleepDelay, false);
                    debugPrint("ms)");
                }
            }
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            turnOff();
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
            turnOff();
    }
    if (m_on && !m_sleeping) {
        sendWiFiCredentials();
    }
}

bool IUESP8285::readMessages()
{
    bool atLeastOneNewMessage = IUSerial::readMessages();
    uint32_t now = millis();
    if (atLeastOneNewMessage) {
        m_lastResponseTime = now;
        if (!m_macAddress) {
            sendMSPCommand(MSPCommand::ASK_WIFI_MAC);
        }
        if (!espFirmwareVersionReceived) {
            sendMSPCommand(MSPCommand::ASK_WIFI_FV);
        }
    } else if (m_on && m_lastResponseTime > 0 &&
               now - m_lastResponseTime > noResponseTimeout) {
        // Ensure your ESP8266 library version is 2.5.0 in .arduino15 folder 
        if (debugMode) {
            debugPrint("WiFi irresponsive: hard resetting now");
        }
        hardReset();
        delay(3000);
        m_lastResponseTime = now;
    }
    if (now - m_lastConnectedStatusTime > connectedStatusTimeout) {
        m_setConnectedStatus(false);
    }
    return atLeastOneNewMessage;
}


/* =============================================================================
    Local storage (flash) management
============================================================================= */

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

bool IUESP8285::configure(JsonVariant &config)
{
    bool success = true;
    const char* tempAuthType = config["auth_type"];
    const char* tempSSID = config["ssid"];
    const char* tempPassword = config["password"];
    const char* tempUsername = config["username"];
    const char* tempStaticIP = config["static"];
    const char* tempGatewayIP = config["gateway"];
    const char* tempSubnetIP = config["subnet"];
    const char* tempDns1 = config["dns1"];
    const char* tempDns2 = config["dns2"];

    setWiFiConfig(credentials::AUTH_TYPE, tempAuthType,strlen(tempAuthType));
    if(strncmp(m_wifiAuthType, "NONE", 4) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, NULL,NULL);
        setWiFiConfig(credentials::USERNAME, NULL,NULL);
    }
    else if(strncmp(m_wifiAuthType, "WPA-PSK", 7) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
        setWiFiConfig(credentials::USERNAME, NULL,NULL);
    }
    else if(strncmp(m_wifiAuthType, "EAP-PEAP", 8) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
        setWiFiConfig(credentials::USERNAME, tempUsername,strlen(tempUsername));
    }
    else if(strncmp(m_wifiAuthType, "EAP-TLS", 7) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
    }
    else if(strncmp(m_wifiAuthType, "STATIC-NONE", 11) == 0)               //to be functional
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, NULL,NULL);
        setWiFiConfig(credentials::USERNAME, NULL,NULL);
        setStaticIP(staticIP::STATIC_IP,tempStaticIP,strlen(tempStaticIP));
        setStaticIP(staticIP::GATEWAY_IP,tempGatewayIP,strlen(tempGatewayIP));
        setStaticIP(staticIP::SUBNET_IP,tempSubnetIP,strlen(tempSubnetIP));
        setStaticIP(staticIP::DNS1_IP,tempDns1,strlen(tempDns1));
        setStaticIP(staticIP::DNS2_IP,tempDns2,strlen(tempDns2));
    }
    else if(strncmp(m_wifiAuthType, "STATIC-WPA-PSK", 14) == 0)            //to be functional
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
        setWiFiConfig(credentials::USERNAME, NULL,NULL);
        setStaticIP(staticIP::STATIC_IP,tempStaticIP,strlen(tempStaticIP));
        setStaticIP(staticIP::GATEWAY_IP,tempGatewayIP,strlen(tempGatewayIP));
        setStaticIP(staticIP::SUBNET_IP,tempSubnetIP,strlen(tempSubnetIP));
        setStaticIP(staticIP::DNS1_IP,tempDns1,strlen(tempDns1));
        setStaticIP(staticIP::DNS2_IP,tempDns2,strlen(tempDns2));
    }
    else if(strncmp(m_wifiAuthType, "STATIC-EAP-PEAP", 15) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
        setWiFiConfig(credentials::USERNAME, tempUsername,strlen(tempUsername));
        setStaticIP(staticIP::STATIC_IP,tempStaticIP,strlen(tempStaticIP));
        setStaticIP(staticIP::GATEWAY_IP,tempGatewayIP,strlen(tempGatewayIP));
        setStaticIP(staticIP::SUBNET_IP,tempSubnetIP,strlen(tempSubnetIP));
        setStaticIP(staticIP::DNS1_IP,tempDns1,strlen(tempDns1));
        setStaticIP(staticIP::DNS2_IP,tempDns2,strlen(tempDns2));
    }
    else if(strncmp(m_wifiAuthType, "STATIC-EAP-TLS", 14) == 0)
    {
        setWiFiConfig(credentials::SSID, tempSSID,strlen(tempSSID));
        setWiFiConfig(credentials::PASSWORD, tempPassword,strlen(tempPassword));
       // setWiFiConfig(credentials::USERNAME, tempUsername,strlen(tempUsername));
        setStaticIP(staticIP::STATIC_IP,tempStaticIP,strlen(tempStaticIP));
        setStaticIP(staticIP::GATEWAY_IP,tempGatewayIP,strlen(tempGatewayIP));
        setStaticIP(staticIP::SUBNET_IP,tempSubnetIP,strlen(tempSubnetIP));
        setStaticIP(staticIP::DNS1_IP,tempDns1,strlen(tempDns1));
        setStaticIP(staticIP::DNS2_IP,tempDns2,strlen(tempDns2));
    }
    else
    {
        success = false;
    }
    if(debugMode) {
        debugPrint(F("SSID      : "),false); debugPrint(m_ssid);
        debugPrint(F("Password  : "),false); debugPrint(m_psk);
        debugPrint(F("Username  : "),false); debugPrint(m_username);
        debugPrint(F("Static IP : "),false); debugPrint(m_staticIP);
        debugPrint(F("Subnet    : "),false); debugPrint(m_staticSubnet);
        debugPrint(F("Gateway   : "),false); debugPrint(m_staticGateway);
        debugPrint(F("Auth Type : "),false); debugPrint(m_wifiAuthType);
        debugPrint(F("DNS 1     : "),false); debugPrint(m_dns1);
        debugPrint(F("DNS 2     : "),false); debugPrint(m_dns2);
    }

    sendWiFiCredentials();
    sendStaticConfig();
    return success;
    
}

// bool IUESP8285::configure(JsonVariant &config)
// {
//     bool success = true;  // success if at least "ssid" and "psk" found
//     const char *value;
// //    value = config.get<const char*>("ssid");
//     value = config["ssid"];
//     if (value) { setSSID(value, strlen(value)); }
//     else { success = false; }
// //    value = config.get<const char*>("psk");
//     value = config["password"];
//     if (value) { setPassword(value, strlen(value)); }
//     else { success = false; }
// //    value = config.get<const char*>("static");
//     value = config["static"];
//     if (value) { setStaticIP(value, strlen(value)); }
// //    value = config.get<const char*>("gateway");
//     value = config["gateway"];
//     if (value) { setGateway(value, strlen(value)); }
// //    value = config.get<const char*>("subnet");
//     value = config["subnet"];
//     if (value) { setSubnetMask(value, strlen(value)); }
//     // Send to WiFi module (consistency check is done inside send functions)
//     m_credentialSent = false;
//     sendWiFiCredentials();
//     sendStaticConfig();
//     return success;
// };

void IUESP8285::setWiFiConfig(credentials type, const char *config, uint8_t length)
{
    if (m_credentialValidator.hasTimedOut())
    {
        m_credentialValidator.reset();
    }
    uint8_t charCount = min(wifiCredentialLength, length);
    switch (type)
    {
    case AUTH_TYPE:
        strncpy(m_wifiAuthType, config, charCount);
        for (uint8_t i = charCount; i < wifiCredentialLength; ++i) {
            m_wifiAuthType[i] = 0;
        }
        break;
    case SSID:
        strncpy(m_ssid, config, charCount);
        for (uint8_t i = charCount; i < wifiCredentialLength; ++i) {
            m_ssid[i] = 0;
        }
        break;   
    case PASSWORD:
        strncpy(m_psk, config, charCount);
        for (uint8_t i = charCount; i < wifiCredentialLength; ++i) {
            m_psk[i] = 0;
        }
        break;
    case USERNAME:
        strncpy(m_username, config, charCount);
        for (uint8_t i = charCount; i < wifiCredentialLength; ++i) {
            m_username[i] = 0;
        }
        break;
    default:
        break;
    }
    m_credentialValidator.receivedMessage(type);
    m_credentialSent = false;
}

void IUESP8285::setStaticIP(staticIP type, IPAddress IP)
{
    if (m_staticConfigValidator.hasTimedOut()) {
        m_staticConfigValidator.reset();
    }
    switch (type)
    {
    case STATIC_IP:
        m_staticIP = IP;
        break;
    case GATEWAY_IP:
        m_staticGateway = IP;
        break;
    case SUBNET_IP:
        m_staticSubnet = IP;
        break;
    case DNS1_IP:
        m_dns1 = IP;
        break;
    case DNS2_IP:
        m_dns2 = IP;
        break;
    default:
        break;
    }
    m_staticConfigValidator.receivedMessage(type);
    m_staticConfigSent = false;
}

bool IUESP8285::setStaticIP(staticIP type, const char *IP, uint8_t len)
{
    IPAddress tempAddress;
    char tempArr[len];
    for (uint8_t i = 0; i < len; ++i) {
        tempArr[i] = IP[i];
    }
    // Make sure the sub string is null terminated.
    tempArr[len] = 0;
    bool success = tempAddress.fromString(tempArr);
    if (success) {
        setStaticIP(type, tempAddress);
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
    // else if (strncmp(buff, "WIFI-SSID-", 10) == 0)
    // {
    //     uint16_t len = strlen(buff);
    //     if (strcmp(&buff[len - 10], "-DISS-IFIW") != 0)
    //     {
    //         if (debugMode)
    //         {
    //             debugPrint("Unparsable SSID");
    //         }
    //         return;
    //     }
    //     setSSID(&buff[10], len - 20);
    //     sendWiFiCredentials();
    //     saveConfigToFlash(iuFlashPtr);
    // }
    // else if (strncmp(buff, "WIFI-PW-", 8) == 0)
    // {
    //     uint16_t len = strlen(buff);
    //     if (strcmp(&buff[len - 8], "-WP-IFIW") != 0)
    //     {
    //         if (debugMode)
    //         {
    //             debugPrint("Unparsable password");
    //         }
    //         return;
    //     }
    //     setPassword(&buff[8], len - 16);
    //     sendWiFiCredentials();
    //     saveConfigToFlash(iuFlashPtr);
    // }
    // else if (strncmp(buff, "WIFI-IP-", 8) == 0)
    // {
    //     uint16_t len = strlen(buff);
    //     if (strcmp(&buff[len - 8], "-PI-IFIW") != 0 ||
    //         !setStaticIP(&buff[8], len - 16))
    //     {
    //         if (debugMode)
    //         {
    //             debugPrint("Unparsable Static IP");
    //         }
    //         return;
    //     }
    //     sendStaticConfig();
    //     saveConfigToFlash(iuFlashPtr);
    // }
    // else if (strncmp(buff, "WIFI-GTW-", 9) == 0)
    // {
    //     uint16_t len = strlen(buff);
    //     if (strcmp(&buff[len - 9], "-WTG-IFIW") != 0 ||
    //         !setGateway(&buff[9], len - 18))
    //     {
    //         if (debugMode)
    //         {
    //             debugPrint("Unparsable Gateway IP");
    //         }
    //         return;
    //     }
    //     sendStaticConfig();
    //     saveConfigToFlash(iuFlashPtr);
    // }
    // else if (strncmp(buff, "WIFI-SM-", 8) == 0)
    // {
    //     uint16_t len = strlen(buff);
    //     if (strcmp(&buff[len - 8], "-MS-IFIW") != 0 ||
    //         !setSubnetMask(&buff[8], len - 16))
    //     {
    //         if (debugMode)
    //         {
    //             debugPrint("Unparsable Subnet Mask IP");
    //         }
    //         return;
    //     }
    //     sendStaticConfig();
    //     saveConfigToFlash(iuFlashPtr);
    // }
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
            m_awakeTimerStart = millis();
            m_lastConnectedStatusTime = m_awakeTimerStart;
            //m_setConnectedStatus(true);         // uncommented temporarily
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_DISCONNECTED"); }
            m_setConnectedStatus(false);
            if (millis() - m_displayConnectAttemptStart >
                displayConnectAttemptTimeout)
            {
                m_working = false;
                m_displayConnectAttemptStart = 0;
            }
            break;
        case MSPCommand::MQTT_ALERT_CONNECTED:
            if (loopDebugMode) { debugPrint("MQTT_ALERT_CONNECTED"); }
            m_awakeTimerStart = millis();
            m_lastConnectedStatusTime = m_awakeTimerStart;
            m_setConnectedStatus(true);
            m_working = false;
            break;
        case MSPCommand::MQTT_ALERT_DISCONNECTED:
            if (loopDebugMode) { debugPrint("MQTT_ALERT_DISCONNECTED"); }
            m_setConnectedStatus(false);
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
            m_sleeping = true;
            break;
        case MSPCommand::WIFI_ALERT_AWAKE:
            if (loopDebugMode) { debugPrint("WIFI_ALERT_AWAKE"); }
            m_sleeping = false;
            break;
        case MSPCommand::WIFI_CONFIRM_PUBLICATION:
            if (loopDebugMode) { debugPrint("WIFI_CONFIRM_PUBLICATION"); }
            m_lastConfirmedPublication = millis();
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
    if (m_credentialValidator.completed() && !m_credentialSent)
    {
        if (loopDebugMode) { debugPrint(F("Sending WiFi Credentials")); }
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_AUTH_TYPE, m_wifiAuthType);
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_SSID, m_ssid);
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_PASSWORD, m_psk);
        // sendMSPCommand(MSPCommand::WIFI_RECEIVE_USERNAME, m_username);
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
    m_credentialSent = false;
}

/**
 *
 */
void IUESP8285::sendStaticConfig()
{
    if (m_staticConfigValidator.completed() && !m_staticConfigSent)
    {
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_STATIC_IP, m_staticIP);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_GATEWAY, m_staticGateway);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_SUBNET, m_staticSubnet);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_DNS1, m_dns1);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_DNS2, m_dns2);
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
    m_staticConfigSent = false;
}

/**
 *
 */
void IUESP8285::connect()
{
    sendMSPCommand(MSPCommand::WIFI_CONNECT);
    delay(10);
    sendMSPCommand(MSPCommand::MQTT_CONNECT);
    m_displayConnectAttemptStart = millis();
    m_working = true;
}

/**
 *
 */
void IUESP8285::disconnect()
{
    sendMSPCommand(MSPCommand::WIFI_DISCONNECT);
    delay(10);
    sendMSPCommand(MSPCommand::MQTT_DISCONNECT);
    m_working = true;
}
