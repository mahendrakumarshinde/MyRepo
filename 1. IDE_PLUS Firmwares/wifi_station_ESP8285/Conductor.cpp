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
        hostSerial->readToBuffer();
        if (!hostSerial->hasNewMessage())
        {
            break;
        }
        processMessageFromHost();
        hostSerial->resetBuffer();  // Clear buffer
    }
}

/**
 *
 */
void Conductor::processMessageFromHost()
{
    MSPCommand::command cmd = hostSerial->getMspCommand();
    char *buffer = hostSerial->getBuffer();
    if (debugMode)
    {
        debugPrint("Received command from host: ", false);
        debugPrint((uint8_t) cmd, false);
        debugPrint(", buffer is: ", false);
        debugPrint(buffer);
    }
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
                enableWifi();
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
                enableWifi();
            }
            break;
        case MSPCommand::WIFI_FORGET_CREDENTIALS:
            forgetWiFiCredentials();
            disableWifi(true, false);
            break;
        case MSPCommand::WIFI_RECEIVE_STATIC_IP:
            if (m_staticConfigValidator.hasTimedOut())
            {
                forgetWiFiStaticConfig();
            }
            if (m_staticIp.fromString(buffer))
            {
                m_staticConfigValidator.receivedMessage(0);
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
        case MSPCommand::WIFI_RECEIVE_GATEWAY:
            if (m_staticConfigValidator.hasTimedOut())
            {
                forgetWiFiStaticConfig();
            }
            if (m_staticIp.fromString(buffer))
            {
                m_staticConfigValidator.receivedMessage(0);
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
        case MSPCommand::WIFI_RECEIVE_SUBNET:
            if (m_staticConfigValidator.hasTimedOut())
            {
                forgetWiFiStaticConfig();
            }
            if (m_staticIp.fromString(buffer))
            {
                m_staticConfigValidator.receivedMessage(0);
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
            // TODO Implement
            break;
        case MSPCommand::WIFI_DEEP_SLEEP:
            // TODO Implement
            break;
        case MSPCommand::WIFI_HARD_RESET:
            ESP.reset();
            break;
        case MSPCommand::WIFI_CONNECT:
            if (debugMode)
            {
                debugPrint("Activating Wifi & using saved credentials...");
            }
            if (!iuWifiManager.hasSavedCredentials())
            {
                hostSerial.sendMSPCommand(
                    MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS);
            }
            else
            {
                iuWifiManager.debugPrintWifiInfo();
                enableWifi();
            }
            break;
        case MSPCommand::WIFI_DISCONNECT:
            disableWifi(true, true);
            break;
        case MSPCommand::PUBLISH_RAW_DATA:

            break;
        case MSPCommand::PUBLISH_FEATURE:

            break;
        case MSPCommand::PUBLISH_DIAGNOSTIC:

            break;
        case MSPCommand::HOST_CONFIRM_RECEPTION:

            break;
        default:

    }

//    else if (strcmp("WIFI-NOSLEEP", buff) == 0)
//    {
//        AUTHORIZED_TO_SLEEP = false;
//        stayAwakeOrderTime = millis();
//        if (debugMode)
//        {
//            debugPrint("Not authorized to sleep");
//        }
//    }
//    else if (strcmp("WIFI-SLEEPOK", buff) == 0)
//    {
//        AUTHORIZED_TO_SLEEP = true;
//        stayAwakeOrderTime = 0;
//        if (debugMode)
//        {
//            debugPrint("Authorized to sleep");
//        }
//    }
//    else if (strncmp("WIFI-DEEPSLEEP-", buff, 15) == 0 && strlen(buff) == 20)
//    {
//        uint64_t durationSec = (uint64_t) atoi(&buff[11]);
//        ESP.deepSleep(durationSec * 1000000, RF_DEFAULT);
//    }
//    // Check if RAW DATA, eg: REC,X,<data>
//    else if (strncmp("REC,", buff, 4) == 0 && buff[5] == ',')
//    {
//        if (accelRawDataHelper.hasTimedOut())
//        {
//            accelRawDataHelper.resetPayload();
//        }
//        accelRawDataHelper.addKeyValuePair(buff[4], &buff[6], strlen(buff) - 6);
//        publishAccelRawDataIfReady();
//    }
//    else if (buff[6] == ',')  // Feature
//    {
//        publishFeature(&buff[7], strlen(buff) - 7, buff, 6);
//    }
//    else if (strncmp(buff, "HB,", 3) == 0 ||
//             strncmp(buff, "DT,", 3) == 0 ||
//             strncmp(buff, "ST,", 3) == 0)  // Diagnsotic
//    {
//        publishDiagnostic(&buff[3], strlen(buff) - 3);
//    }
//    else if (debugMode)
//    {
//        debugPrint("Unknown message from host");
//    }
}



/* =============================================================================
    WiFi credentials and config
============================================================================= */

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
    m_staticIp = IPAddress(0);
    m_staticGateway = IPAddress(0);
    m_staticSubnet = IPAddress(0);
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
        m_lastConnected = current;
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
    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
    }
    if (forceNewCredentials)
    {
        String currentSSID = WiFi.SSID();
        String currentPW = WiFi.psk();
        if (currentSSID.length() > 0 &&
            strcmp(currentSSID.c_str(), currentSSID) == 0 &&
            currentPW.length() > 0 &&
            strcmp(currentPW.c_str(), m_userPassword) == 0)
        {
            // User input SSID & Password are the same than current
        }
        else
        {
            // New and different user input for SSID and Password => disconnect
            // from current SSID then reconnect to new SSID
            disconnectWifi();
            delay(1000);  // Wait for effective disconnection
        }
    }
    uint32_t current = millis();
    if (WiFi.isConnected())
    {
        // Already connected with the right credentials
        return true;
    }
    else
    {
        WiFi.begin(m_userSSID, m_userPassword);
        m_lastConnectionAttempt = current;
        if (m_remainingConnectionAttempt > 0)
        {
            m_remainingConnectionAttempt--;
        }
        bool connectSuccess = (waitForConnectResult() == WL_CONNECTED);
        if (connectSuccess && debugMode)
        {
            debugPrint("Connected to ", false);
            debugPrint(WiFi.SSID());
        }
        return connectSuccess;
    }
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
        return status;
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
    Main operations
============================================================================= */


void Conductor::setup()
{
    hostSerial.begin();
    hostSerial.port->flush();
    turnOffRadio();
    /***** Request host to know if should sleep & get BLE MAC address *****/
    uint32_t startTime = millis();
    uint32_t current = startTime;
    while (!m_shouldWakeUp || unknownBleMacAddress)
    {
        if (startTime - current < hostResponseTimeout)
        {
            ESP.deepSleep(deepSleepDuration);  // in micro seconds
        }
        hostSerial.sendMSPCommand(WIFI_REQUEST_ACTION);
        hostSerial.sendMSPCommand(ASK_BLE_MAC);
        readMessagesFromHost();
        delay(100);
        current = millis();
    }
    /***** Get device (=BLE) MAC address*****/
    current = startTime = millis();
    while (unknownBleMacAddress)
    {
        hostSerial.sendMSPCommand(ASK_BLE_MAC);
        readMessagesFromHost();
        delay(100);
        current = millis();
    }


}

void Conductor::loop()
{

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


