#include "Conductor.h"


/* =============================================================================
    Instanciation
============================================================================= */

char hostSerialBuffer[3072];
IUSerial hostSerial(&Serial, hostSerialBuffer, 3072, IUSerial::MS_PROTOCOL,
                    115200, ';', 100);

IURawDataHelper accelRawDataHelper(10000);  // 10s timeout

IUMQTTHelper mqttHelper;

IUTimeHelper timeHelper(2390, "time.google.com");


/* =============================================================================
    Conductor
============================================================================= */

Conductor::Conductor() :
    m_unknownBleMacAddress(true),
    m_shouldWakeUp(false),
    m_radioOn(true),
    m_lastConnectionAttempt(0),
    m_lastConnected(0),
    m_remainingConnectionAttempt(connectionRetry)
{
    strncpy(m_bleMacAddress, "00:00:00:00:00:00", 18);
    strncpy(m_wifiMacAddress, "00:00:00:00:00:00", 18);
    m_credentialValidator.setTimeout(wifiConfigReceptionTimeout);
    m_staticConfigValidator.setTimeout(wifiConfigReceptionTimeout);
}

/**
 *
 *
 * @param duration_ms The sleep duration in microseconds. If duration_ms=0, will
 *  sleep for default Conductor::deepSleepDuration.
 */
void Conductor::deepsleep(uint32_t duration_ms)
{
    if (debugMode)
    {
        debugPrint("Going to deep-sleep");
    }
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_SLEEPING);
    delay(100); // Wait to send serial messages
    ESP.deepSleep(duration_ms * 1000);
}


/* =============================================================================
    Communication with host
============================================================================= */


/**
 * Read from host serial and process the command, if one was received.
 */
void Conductor::readMessagesFromHost()
{
    while(true)
    {
        hostSerial.readToBuffer();
        if (!hostSerial.hasNewMessage())
        {
            break;
        }
        processMessageFromHost();
        hostSerial.resetBuffer();  // Clear buffer
    }
}

/**
 *
 */
void Conductor::processMessageFromHost()
{
    MSPCommand::command cmd = hostSerial.getMspCommand();
    char *buffer = hostSerial.getBuffer();
    uint16_t bufferLength = hostSerial.getCurrentBufferLength();
    if (debugMode)
    {
        debugPrint("Received command from host: ", false);
        debugPrint((uint8_t) cmd, false);
        debugPrint(", buffer is: ", false);
        debugPrint(buffer);
    }
    uint8_t idx = 0;
    IPAddress *ipPtr = NULL;
    switch(cmd)
    {
        case MSPCommand::RECEIVE_BLE_MAC:
            strncpy(m_bleMacAddress, buffer, 18);
            m_bleMacAddress[17] = 0; // Make sure end of string is set.
            if (debugMode)
            {
                debugPrint("Received BLE MAC from host: ", false);
                debugPrint(m_bleMacAddress);
            }
            mqttHelper.setDeviceMacAddress(m_bleMacAddress);
            m_unknownBleMacAddress = false;
            break;
        case MSPCommand::ASK_WIFI_MAC:
            hostSerial.sendMSPCommand(MSPCommand::RECEIVE_WIFI_MAC,
                                      m_wifiMacAddress);
            break;
        case MSPCommand::WIFI_RECEIVE_SSID:
            if (m_credentialValidator.hasTimedOut())
            {
                forgetWiFiCredentials();
            }
            strncpy(m_userSSID, buffer, wifiCredentialLength);
            m_credentialValidator.receivedMessage(0);
            if (m_credentialValidator.completed())
            {
                reconnect(true);
            }
            break;
        case MSPCommand::WIFI_RECEIVE_PASSWORD:
            if (m_credentialValidator.hasTimedOut())
            {
                forgetWiFiCredentials();
            }
            strncpy(m_userPassword, buffer, wifiCredentialLength);
            m_credentialValidator.receivedMessage(1);
            if (m_credentialValidator.completed())
            {
                reconnect(true);
            }
            break;
        case MSPCommand::WIFI_FORGET_CREDENTIALS:
            forgetWiFiCredentials();
            disconnectWifi();
            break;
        case MSPCommand::WIFI_RECEIVE_STATIC_IP:
        case MSPCommand::WIFI_RECEIVE_GATEWAY:
        case MSPCommand::WIFI_RECEIVE_SUBNET:
            idx = (uint8_t) cmd - (uint8_t) MSPCommand::WIFI_RECEIVE_STATIC_IP;
            if (idx == 0) { ipPtr = &m_staticIp; }
            else if (idx == 1) { ipPtr = &m_staticGateway; }
            else if (idx == 2) { ipPtr = &m_staticSubnet; }
            else { break; }
            if (m_staticConfigValidator.hasTimedOut())
            {
                forgetWiFiStaticConfig();
            }
            if (ipPtr->fromString(buffer))
            {
                m_staticConfigValidator.receivedMessage(idx);
            }
            else if (debugMode)
            {
                debugPrint("Couldn't parse IP address: ", false);
                debugPrint(buffer);
            }
            if (m_staticConfigValidator.completed())
            {
                // TODO Implement
            }
            break;
        case MSPCommand::WIFI_FORGET_STATIC_CONFIG:
            forgetWiFiStaticConfig();
            break;
        case MSPCommand::WIFI_WAKE_UP:
            m_shouldWakeUp = true;
            break;
        case MSPCommand::WIFI_DEEP_SLEEP:
            m_shouldWakeUp = false;
            deepsleep();
            break;
        case MSPCommand::WIFI_HARD_RESET:
            ESP.reset();
            break;
        case MSPCommand::WIFI_CONNECT:
            if (m_credentialValidator.completed())
            {
                reconnect();
            }
            else
            {
                hostSerial.sendMSPCommand(
                    MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS);
            }
            break;
        case MSPCommand::WIFI_DISCONNECT:
            disconnectWifi();
            break;
        case MSPCommand::PUBLISH_RAW_DATA:
            if (accelRawDataHelper.hasTimedOut())
            {
                accelRawDataHelper.resetPayload();
            }
            accelRawDataHelper.addKeyValuePair(buffer[0], &buffer[2],
                                               strlen(buffer) - 2);
            accelRawDataHelper.publishIfReady(m_bleMacAddress);
            break;
        case MSPCommand::PUBLISH_FEATURE:
            mqttHelper.publishFeature(buffer, bufferLength);
            break;
        case MSPCommand::PUBLISH_DIAGNOSTIC:
            mqttHelper.publishDiagnostic(buffer, bufferLength,
                                         timeHelper.getCurrentTime());
            break;
        case MSPCommand::HOST_CONFIRM_RECEPTION:
            // TODO Implement
            break;
    }
}



/* =============================================================================
    WiFi credentials and config
============================================================================= */

void Conductor::setCredentials(const char *userSSID, const char *userPSK)
{
    strncpy(m_userSSID, userSSID, wifiCredentialLength);
    strncpy(m_userPassword, userPSK, wifiCredentialLength);
    m_credentialValidator.receivedMessage(0);
    m_credentialValidator.receivedMessage(1);
}

/**
 *
 */
void Conductor::forgetWiFiCredentials()
{
    m_credentialValidator.reset();
    for (uint8_t i = 0; i < wifiCredentialLength; ++i)
    {
        m_userSSID[i] = 0;
        m_userPassword[i] = 0;
    }
}

/**
 *
 */
void Conductor::forgetWiFiStaticConfig()
{
    m_staticConfigValidator.reset();
    m_staticIp = IPAddress();
    m_staticGateway = IPAddress();
    m_staticSubnet = IPAddress();
}

/**
 * Request host to know if should sleep & get BLE MAC address.
 *
 * If the host has not answered after hostResponseTimeout, the ESP82 will deep-
 * sleep for deepSleepDuration and then restart.
 */
bool Conductor::getConfigFromMainBoard()
{
    uint32_t startTime = millis();
    uint32_t current = startTime;
    uint16_t i = 0;
    while (!m_shouldWakeUp || m_unknownBleMacAddress)
    {
        if (current - startTime > hostResponseTimeout)
        {
            if (debugMode)
            {
                debugPrint("Host didn't respond");
            }
            deepsleep();
        }
        hostSerial.sendMSPCommand(MSPCommand::WIFI_REQUEST_ACTION);
        hostSerial.sendMSPCommand(MSPCommand::ASK_BLE_MAC);
        readMessagesFromHost();
        delay(100);
        current = millis();
    }
    return (m_shouldWakeUp && m_unknownBleMacAddress);
}


/* =============================================================================
    Wifi connection and status
============================================================================= */

/**
 * Turn off the WiFi radio for the given duration (in micro-seconds)
 *
 * @param duration_us The duration for which the WiFi radio should be turned
 *  off. If duration_us = 0, WiFi radio will be turned off indefinitely.
 */
void Conductor::turnOffRadio(uint32_t duration_us)
{
    if (m_radioOn)
    {
        m_radioOn = false;
        disconnectWifi();  // Make sure WiFi is disconnected
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin(duration_us);
        yield();  // better than delay(1); ?
    }
}

/**
 * Turn off the WiFi radio for the given duration (in micro-seconds)
 */
void Conductor::turnOnRadio()
{
    if (!m_radioOn)
    {
        m_radioOn = true;
        WiFi.forceSleepWake();
        yield();  // better than delay(1); ?
    }
}

/**
 * Disconnect the clients and the WiFi
 *
 * @param wifiOff If true, set WiFi mode to WIFI_OFF.
 */
void Conductor::disconnectWifi(bool wifiOff)
{
    /***** End NTP server connection *****/
    timeHelper.end();
    /***** Disconnect MQTT client *****/
    if (mqttHelper.client.connected())
    {
        mqttHelper.client.disconnect();
    }
    /***** Turn off Wifi *****/
    if (WiFi.isConnected())
    {
        WiFi.disconnect(wifiOff);
    }
}

/**
 * Attempt to connect or reconnect to the WiFi
 *
 * If the WiFi is already connected to the "right" network (see param
 * forceNewCredentials), this function will not affect the Wifi connection.
 *
 * @param forceNewCredentials If true, the function will connect to the
 * user-input SSID and password (m_userSSID and m_userPassword). If the WiFi is
 * already connected to a network with a different SSID and password, this
 * former connection will be interrupted to use the new credentials.
 */
bool Conductor::reconnect(bool forceNewCredentials)
{
    // Ensure that the WiFi is in AP mode
    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
    }
    // Disconnect the WiFi if new credentials
    if (forceNewCredentials)
    {
        String currentSSID = WiFi.SSID();
        String currentPW = WiFi.psk();
        if (currentSSID.length() == 0 ||
            strcmp(currentSSID.c_str(), m_userSSID) > 0 ||
            currentPW.length() == 0 ||
            strcmp(currentPW.c_str(), m_userPassword) > 0)
        {
            // New and different user input for SSID and Password => disconnect
            // from current SSID then reconnect to new SSID
            disconnectWifi();
            delay(1000);  // Wait for effective disconnection
        }
    }
    uint32_t current = millis();
    // Connect the WiFi if not connected
    bool wifiConnected = WiFi.isConnected();
    if (!wifiConnected)
    {
        if (current - m_lastConnectionAttempt < reconnectionInterval)
        {
            if (debugMode)
            {
                debugPrint("Not enough time since last connection attempt");
            }
            return false;
        }
        if (m_remainingConnectionAttempt <= 0)
        {
            if (debugMode)
            {
                debugPrint("No remaining connection attempt");
            }
            return false;
        }
        if (!m_credentialValidator.completed())
        {
            if (debugMode)
            {
                debugPrint("Can't connect without credentials");
            }
            return false;
        }
        WiFi.begin(m_userSSID, m_userPassword);
        m_lastConnectionAttempt = current;
        m_remainingConnectionAttempt--;
        wifiConnected = (waitForConnectResult() == WL_CONNECTED);
        if (debugMode && wifiConnected)
        {
            debugPrint("Connected to ", false);
            debugPrint(WiFi.SSID());
        }
    }
    // Set light sleep mode if not done
    if (wifiConnected && WiFi.getSleepMode() != WIFI_LIGHT_SLEEP)
    {
        WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
    }
    return wifiConnected;
}

/**
 * Wait for WiFi connection to reach a result
 *
 * Unlike the ESP function WiFi.waitForConnectResult, this one handles a
 * connection time-out. If the timeout=0, then it just calls the ESP library
 * function.
 *
 * @return The status reached or disconnect if STA is off
 */
uint8_t Conductor::waitForConnectResult()
{
    if (connectionTimeout == 0)
    {
        return WiFi.waitForConnectResult();  // Use ESP function
    }
    else
    {
        uint32_t startT = millis();
        uint32_t current = startT;
        uint8_t status = WiFi.status();
        while(current - startT < connectionTimeout &&
              status != WL_CONNECTED && status != WL_CONNECT_FAILED)
        {
            delay(100);
            status = WiFi.status();
            current = millis();
        }
        if (debugMode && current - startT > connectionTimeout)
        {
            debugPrint("WiFi connection time-out");
        }
        return status;
    }
}

/**
 *
 */
void Conductor::checkWiFiDisconnectionTimeout()
{
    uint32_t now = millis();
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        m_lastConnected = now;
    }
    else if (now - m_lastConnected > disconnectionTimeout)
    {
        if (debugMode)
        {
            debugPrint("Exceeded disconnection time-out");
        }
        deepsleep();
    }
}


/* =============================================================================
    MQTT
============================================================================= */

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * The message can come from any previously subscribed topic (see
 * PubSubClient.subscribe).
 * @param topic The Pubsub topic the message came from
 * @param payload The message it self, as an array of bytes
 * @param length The number of byte in payload
 */
void Conductor::processMessageFromMQTT(char* topic, byte* payload,
                                       uint16_t length)
{
    if (debugMode)
    {
        debugPrint("MQTT message arrived [", false);
        debugPrint(topic, false);
        debugPrint("] ", false);
        for (int i = 0; i < length; i++)
        {
            debugPrint((char)payload[i], false);
        }
        debugPrint("");
    }
    char *subTopic;
    subTopic = strrchr(topic, '/');
    if (subTopic == NULL || strlen(subTopic) < 2)
    {
        if (debugMode)
        {
            debugPrint("Last level of MQTT topic not found");
        }
        return;
    }
    if (debugMode)
    {
        debugPrint("Command type is: ", false);
        debugPrint(&subTopic[1]);
    }
    if (strncmp(&subTopic[1], "config", 6) == 0)
    {
        // TODO Implement
    }
    else if (strncmp(&subTopic[1], "time_sync", 9) == 0)
    {
        // TODO Implement
    }
    else if (strncmp(&subTopic[1], "legacy", 6) == 0)
    {
        for (int i = 0; i < length; i++)
        {
            hostSerial.port->print((char)payload[i]);
        }
        hostSerial.port->print(";");
        if ((char)payload[0] == '1' && (char)payload[1] == ':')
        {
            timeHelper.updateTimeReferenceFromIU(payload, length);
        }
    }
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
void Conductor::getWifiInfo(char *destination, bool mqttOn)
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
void Conductor::debugPrintWifiInfo()
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
    debugPrint(m_userSSID);
    debugPrint("User psk: ", false);
    debugPrint(m_userPassword);
    #endif
}


