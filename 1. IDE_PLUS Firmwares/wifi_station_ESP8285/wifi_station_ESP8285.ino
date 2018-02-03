/*
  Infinite Uptime BLE Module Firmware

  Update 2017-12-06
*/

#include "IUMQTTHelper.h"
#include "IUWiFiManager.h"
#include "IUSerial.h"
#include "TimeManager.h"
#include "IURawDataHandler.h"

/* =============================================================================
    Global Configuration Variables
============================================================================= */

// Device Type for command subscription
char DEVICE_TYPE[9] = "ide_plus";

// Customer name placeholder, necessary to comply to PubSub expected payload format
char CUSTOMER_PLACEHOLDER[9] = "XXXAdmin";

// BLE MAC Address (used as device MAC address)
char BLE_MAC_ADDRESS[18] = "";                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
bool unknownBleMacAddress = true;

// Access point name
char AP_NAME[30] = "IUsetup";
const char AP_NAME_PREFIX[9] = "IUsetup-";

char WILL_MESSAGE[44] = "XXXAdmin;;;00:00:00:00:00:00;;;disconnected";


/* =============================================================================
    Global Operation Variables
============================================================================= */

bool AUTHORIZED_TO_SLEEP = true;
bool WIFI_ENABLED = true;

uint32_t STAY_AWAKE_DURATION = 600000;  // ms
uint32_t stayAwakeOrderTime = 0;
uint64_t DEEP_SLEEP_DURATION = 30000000;  // in micro seconds

uint32_t WIFI_STATUS_REFRESH_INTERVAL = 10000;  // ms
uint32_t lastWifiStatusRefresh = 0;

char hostSerialBuffer[3072];
IUSerial hostSerial(&Serial, hostSerialBuffer, 3072, IUSerial::LEGACY_PROTOCOL,
                    115200, ';', 100);


/* =============================================================================
    MQTT message reception callback
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
void mqttNewMessageCallback(char* topic, byte* payload, uint16_t length)
{
    if (debugMode)
    {
        debugPrint("Message arrived [", false);
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
            timeManager.updateTimeReferenceFromIU(payload, length);
        }
    }
}

/**
 * 
 */
bool publishDiagnostic(const char *rawMsg, const uint16_t msgLength,
                       const char *topicExtension=NULL,
                       const uint16_t extensionLength=0)
{
    uint16_t nameLength = strlen(CUSTOMER_PLACEHOLDER);
    char message[msgLength + nameLength + 51];
    time_t datetime = timeManager.getCurrentTime();
    strcpy(message, CUSTOMER_PLACEHOLDER);
    strcat(message, ";;;");
    strcat(message, BLE_MAC_ADDRESS);
    strcat(message, ";;;");
    strcat(message, ctime(&datetime));
    strcat(message, ";;;");
    strncat(message, rawMsg, msgLength);
    if (topicExtension && extensionLength > 0)
    {
        char topic[DIAGNOSTIC_TOPIC_LENGTH + extensionLength + 1];
        strcpy(topic, DIAGNOSTIC_TOPIC);
        strcat(topic, "/");
        strncat(topic, topicExtension, extensionLength);
        return iuMQTTHelper.publish(topic, message);
    }
    else
    {
        return iuMQTTHelper.publish(DIAGNOSTIC_TOPIC, message);
    }
}

/**
 * 
 */
bool publishFeature(const char *rawMsg, const uint16_t msgLength,
                    const char *topicExtension=NULL,
                    const uint16_t extensionLength=0)
{
    uint16_t nameLength = strlen(CUSTOMER_PLACEHOLDER);
    char message[msgLength + nameLength + 24];
    strcpy(message, CUSTOMER_PLACEHOLDER);
    strcat(message, ";;;");
    strcat(message, BLE_MAC_ADDRESS);
    strcat(message, ";;;");
    strncat(message, rawMsg, msgLength);
    if (topicExtension && extensionLength > 0)
    {
        char topic[FEATURE_TOPIC_LENGTH + extensionLength + 1];
        strcpy(topic, FEATURE_TOPIC);
        strcat(topic, "/");
        strncat(topic, topicExtension, extensionLength);
        return iuMQTTHelper.publish(topic, message);
    }
    else
    {
        return iuMQTTHelper.publish(DIAGNOSTIC_TOPIC, message);
    }
}

/**
 * 
 */
bool publishAccelRawDataIfReady()
{
    if (accelRawDataHandler.hasTimedOut())
    {
        accelRawDataHandler.resetPayload();
        return false;
    }
    if (!accelRawDataHandler.areAllKeyPresent())
    {
        // Raw Data payload is not ready
        return false;
    }
    int httpCode = accelRawDataHandler.httpPostPayload(BLE_MAC_ADDRESS);
    if (debugMode)
    {
        debugPrint("Post raw data: ", false);
        debugPrint(httpCode);
    }
    if (httpCode == 200)
    {
        accelRawDataHandler.resetPayload();
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Subscribe to all the required device subscriptions
 *
 * Should be called after each reconnection.
 * This function should be edited when new subscriptions are required for the
 * device.
 */
void onMQTTConnection()
{
    iuMQTTHelper.subscribe("config");  // Config subscription
    iuMQTTHelper.subscribe("time_sync");  // Time synchornisation subscription
    iuMQTTHelper.subscribe("legacy");  // Legacy command format subscription
    publishDiagnostic("connected", 9);
}

/* =============================================================================
    Communication to main board functions
============================================================================= */

/**
 * 
 */
void askBleMacAddress()
{
    hostSerial.port->print("77;");
}

/**
 * 
 */
void sendWifiStatus(bool isConnected)
{
    if (isConnected)
    {
        hostSerial.port->print("88-OK;");
    }
    else
    {
        hostSerial.port->print("88-NOK;");
    }
}

bool canSleep()
{
    if (!AUTHORIZED_TO_SLEEP)
    {
        if (stayAwakeOrderTime + STAY_AWAKE_DURATION < millis())
        {
            AUTHORIZED_TO_SLEEP = true;
            stayAwakeOrderTime = 0;
        }
    }
    return AUTHORIZED_TO_SLEEP;
}

/**
 * 
 * Get current status, send message to host (STM32) and server, and reset
 * the ESP if cannot get connection for more than timeout.
 */
bool maintainConnectionStatus()
{
    bool connectStatus = (WiFi.isConnected() &&
                          iuMQTTHelper.client.connected());
    uint32_t now = millis();
    if (lastWifiStatusRefresh == 0 ||
        now > lastWifiStatusRefresh + WIFI_STATUS_REFRESH_INTERVAL ||
        now < lastWifiStatusRefresh)
    {
        if (connectStatus)
        {
            sendWifiStatus(true);
            iuWifiManager.hasTimedOut(true);
        }
        else
        {
            sendWifiStatus(false);
            if (WIFI_ENABLED)
            {
                bool timedOut = iuWifiManager.hasTimedOut(false);
                if (timedOut)
                {
                    if (debugMode)
                    {
                        debugPrint(millis(), false);
                        debugPrint(": Reached WiFi Manager timeout");
                    }
//                    ESP.reset();
                    if (canSleep())
                    {
                        if (debugMode)
                        {
                            debugPrint("Sleeping...");
                            delay(1);
                        }
                        ESP.deepSleep(DEEP_SLEEP_DURATION);  // in micro seconds
                    }
//                    sendWifiStatus(false);
//                    uint32_t sleepEnd1 = millis() + 1000;
//                    while (millis() < sleepEnd1 && Serial.available() == 0)
//                    {
//                        delay(10);
//                    }
//                    if (Serial.available() == 0)
//                    {
//                        iuWifiManager.saveCurrentCredentials();
//                        WiFi.mode(WIFI_OFF);
//                        uint32_t sleepEnd2 = millis() + 50000;
//                        while (millis() < sleepEnd2 && Serial.available() == 0)
//                        {
//                            delay(10);
//                        }
//                    }
                }
            }
            else
            {
                // Not connected, but wifi is disabled => do not time out
                iuWifiManager.hasTimedOut(true);
            }
        }
        lastWifiStatusRefresh = now;
    }
    // Send message to server
    if (iuWifiManager.isTimeToSendWifiInfo())
    {
        if (connectStatus)
        {
            char message[256];
            iuWifiManager.getWifiInfo(message, true);
            publishDiagnostic(message, strlen(message));
        }
        iuWifiManager.debugPrintWifiInfo();
    }
    return connectStatus;
}

/**
 * 
 */
void readAllMessagesFromHost()
{
    while(true)
    {
        hostSerial.readToBuffer();
        if (!hostSerial.hasNewMessage())
        {
            break;
        }
        processMessageFromHost(hostSerial.getBuffer());
        hostSerial.resetBuffer();
    }
}

/**
 * 
 */
void processMessageFromHost(char *buff)
{
    if (strlen(buff) == 0)
    {
        if (debugMode)
        {
            debugPrint("Received empty buffer from host");
        }
        return;
    }
    if (debugMode)
    {
        debugPrint("Process message: ", false);
        debugPrint(buff);
    }
    // Check if it is the BLE MAC in format "BLEMAC-XX:XX:XX:XX:XX:XX"
    if (strlen(buff) == 24 && ( strncmp("BLEMAC-", buff, 7) == 0 ))
    {
        strncpy(BLE_MAC_ADDRESS, &buff[7], 18);
        // Update AP_NAME_PREFIX;
        strcpy(AP_NAME, AP_NAME_PREFIX);
        strcat(AP_NAME, BLE_MAC_ADDRESS);
        // Update MQTT last will message
        char *pch;
        pch = strstr(WILL_MESSAGE, "00:00:00:00:00:00");
        if (pch)
        {
            strncpy(pch, BLE_MAC_ADDRESS, 17);
        }
        if (debugMode)
        {
            debugPrint("MQTT last will updated: ", false);
            debugPrint(WILL_MESSAGE);
        }
        iuMQTTHelper.setDeviceInfo(DEVICE_TYPE, BLE_MAC_ADDRESS);
        unknownBleMacAddress = false;
        if (debugMode)
        {
            debugPrint("Received BLE MAC from host: ", false);
            debugPrint(BLE_MAC_ADDRESS);
        }
    }
    // Reset the ESP - Typically sent by the STM32 when it restarts
    else if (strcmp("WIFI-HARDRESET", buff) == 0)
    {
        ESP.reset();
    }
    else if (strcmp(buff, "WIFI-USE-SAVED") == 0)
    {
        if (debugMode)
        {
            debugPrint("Activating Wifi & using saved credentials...");
        }
        if (!iuWifiManager.hasSavedCredentials())
        {
            hostSerial.port->print("WIFI-NOSAVEDCRED;");
        }
        else
        {
            iuWifiManager.debugPrintWifiInfo();
            enableWifi();
        }
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
        enableWifi();
        iuWifiManager.addUserSSID(&buff[10], len - 20);
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
        enableWifi();
        iuWifiManager.addUserPassword(&buff[8], len - 16);
    }
    else if (strcmp(buff, "WIFI-DISABLE") == 0)
    {
        disableWifi(true, true);
    }
    else if (strcmp(buff, "WIFI-FORGET") == 0)
    {
        disableWifi(true, false);
    }
    else if (strcmp("WIFI-NOSLEEP", buff) == 0)
    {
        AUTHORIZED_TO_SLEEP = false;
        stayAwakeOrderTime = millis();
        if (debugMode)
        {
            debugPrint("Not authorized to sleep");
        }
    }
    else if (strcmp("WIFI-SLEEPOK", buff) == 0)
    {
        AUTHORIZED_TO_SLEEP = true;
        stayAwakeOrderTime = 0;
        if (debugMode)
        {
            debugPrint("Authorized to sleep");
        }
    }
    else if (strncmp("WIFI-DEEPSLEEP-", buff, 15) == 0 && strlen(buff) == 20)
    {
        uint64_t durationSec = (uint64_t) atoi(&buff[11]);
        ESP.deepSleep(durationSec * 1000000, RF_DEFAULT);
    }
    // Check if RAW DATA, eg: REC,X,<data>
    else if (strncmp("REC,", buff, 4) == 0 && buff[5] == ',')
    {
        if (accelRawDataHandler.hasTimedOut())
        {
            accelRawDataHandler.resetPayload();
        }
        accelRawDataHandler.addKeyValuePair(buff[4], &buff[6], strlen(buff) - 6);
        publishAccelRawDataIfReady();
    }
    else if (buff[6] == ',')  // Feature
    {
        publishFeature(&buff[7], strlen(buff) - 7, buff, 6);
    }
    else if (strncmp(buff, "HB,", 3) == 0 ||
             strncmp(buff, "DT,", 3) == 0 ||
             strncmp(buff, "ST,", 3) == 0)  // Diagnsotic
    {
        publishDiagnostic(&buff[3], strlen(buff) - 3);
    }
    else if (debugMode)
    {
        debugPrint("Unknown message from host");
    }
}


/* =============================================================================
    Connection management function
============================================================================= */

void enableWifi()
{
    if (!WIFI_ENABLED && debugMode)
    {
        debugPrint("Wifi is enabled.");
    }
    WIFI_ENABLED = true;
}

/**
 * Disconnect the clients and the WiFi
 * 
 * @param turnOffWiFi If true, the WiFi will go off (no STA, no AP). That
 * also make the ESP8285 "forget" the last saved SSID + credentials.
 */
void disableWifi(bool turnOffWiFi, bool retainCredentials)
{
    if (retainCredentials)
    {
        iuWifiManager.saveCurrentCredentials();
    }
    else
    {
        iuWifiManager.resetUserSSID();
        iuWifiManager.resetUserPassword();
    }
    /***** End NTP server connection *****/
    timeManager.end();
    /***** Disconnect MQTT client *****/
    if (iuMQTTHelper.client.connected())
    {
        iuMQTTHelper.client.disconnect();
    }
    /***** Turn off Wifi *****/
    if (WiFi.isConnected())
    {
        WiFi.disconnect(turnOffWiFi);
    }
    if (WIFI_ENABLED && debugMode)
    {
        debugPrint("Wifi is disabled.");
    }
    sendWifiStatus(false);
    WIFI_ENABLED = false;
}


/* =============================================================================
    Main setup and loop
============================================================================= */

void setup()
{
    hostSerial.begin();
    /***** Get device (=BLE) MAC address*****/
    while (unknownBleMacAddress)
    {
        // Do not start normal operation until BLE MAC Address is known.
        askBleMacAddress();
        readAllMessagesFromHost();
        delay(100);
    }
    /***** Turn on WiFi *****/
    if (WiFi.getMode() == WIFI_AP)
    {
        WiFi.mode(WIFI_OFF);
    }
    if (iuWifiManager.hasSavedCredentials())
    {
        if (debugMode)
        {
            debugPrint("Saved credentials found: starting WiFi...");
        }
        enableWifi();
    }
    else
    {
        if (debugMode)
        {
            debugPrint("Saved credentials not found: stopping WiFi...");
        }
        disableWifi(true, false);
    }
    /***** Prepare to receive MQTT messages *****/
    iuMQTTHelper.client.setCallback(mqttNewMessageCallback);
    delay(1000);
    // Reset WiFiManager timer
    iuWifiManager.hasTimedOut(true);
    if (debugMode)
    {
        debugPrint("Ended setup at ", false);
        debugPrint(millis(), false);
        debugPrint(';');
    }
//    Serial.print("Ended setup at ");
//    Serial.print(millis());
//    Serial.println(';');
}

void loop()
{
    /***** WiFi connection maintenance *****/
    if (WIFI_ENABLED && iuWifiManager.reconnect())
    {
        /***** Sleep mode *****/
        if (WiFi.getSleepMode() != WIFI_LIGHT_SLEEP)
        {
            if (!WiFi.setSleepMode(WIFI_LIGHT_SLEEP))
            {
                if (debugMode)
                {
                    debugPrint("Failed to set light sleep mode");
                }
            }
        }
        /***** Time management (from IU and / or NTP server) *****/
        if (!timeManager.active())
        {
            //Prepare to query time from NTP server
            timeManager.begin();
        }
        timeManager.updateTimeReferenceFromNTP();
        /***** MQTT Connection loop *****/
        iuMQTTHelper.loop(DIAGNOSTIC_TOPIC, WILL_MESSAGE, onMQTTConnection,
                          1000);
        // Publish raw data (HTTP POST request)
        publishAccelRawDataIfReady();
    }
    /***** Maintain wifi status and send it to host and server *****/
    maintainConnectionStatus();
    /***** Read message from main board and process them *****/
    readAllMessagesFromHost();
    /***** Light sleep (and listen to serial) *****/
    uint32_t sleepEnd = millis() + 1000;
    while (millis() < sleepEnd && Serial.available() == 0)
    {
        delay(10);
    }
}

