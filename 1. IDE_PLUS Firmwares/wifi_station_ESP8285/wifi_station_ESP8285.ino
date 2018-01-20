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
    Global Variables
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

bool AUTHORIZED_TO_SLEEP = true;

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
    // Config subscription
    iuMQTTHelper.subscribe("config");
    // Time synchornisation subscription
    iuMQTTHelper.subscribe("time_sync");
    // Legacy command format subscription
    iuMQTTHelper.subscribe("legacy");
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
    if(isConnected)
    {
        hostSerial.port->print("88-OK;");
    }
    else
    {
        hostSerial.port->print("88-NOK;");
    }
}

/**
 * 
 */
void onWifiConnection()
{
    sendWifiStatus(true);
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
//        ESP.restart();
    }
    //
    else if (strcmp("WIFI-NOSLEEP", buff) == 0)
    {
        AUTHORIZED_TO_SLEEP = false;
    }
    //
    else if (strcmp("WIFI-SLEEPOK", buff) == 0)
    {
        AUTHORIZED_TO_SLEEP = true;
    }
    //
    else if (strncmp("WIFI-SLEEP-", buff, 11) == 0 && strlen(buff) == 18)
    {
        uint16_t duration = (uint16_t) atoi(&buff[11]);
        delay(duration);
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
             strncmp(buff, "DT,", 3) == 0)  // Diagnsotic
    {
        publishDiagnostic(&buff[3], strlen(buff) - 3);
    }
    else if (debugMode)
    {
        debugPrint("Unknown message from host");
    }
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
    /***** Connect to WiFi for the first time *****/
    iuWifiManager.manageWifi(AP_NAME, NULL, onWifiConnection);
    if (!WiFi.setSleepMode(WIFI_LIGHT_SLEEP))
    {
        if (debugMode)
        {
            debugPrint("Setup - Failed to set light sleep mode");
        }
    }
    /***** Prepare to receive MQTT messages *****/
    iuMQTTHelper.client.setCallback(mqttNewMessageCallback);
    /***** Prepare to query time from NTP server *****/
    timeManager.begin();
    // Try to get the time from NTP server at least once
    timeManager.updateTimeReferenceFromNTP();
    delay(1000);
}


void loop()
{
    /***** Wifi Connection loop *****/
    if(!iuWifiManager.reconnect(AP_NAME, NULL, onWifiConnection))
    {
        sendWifiStatus(false);
        return;
    }
    else if (WiFi.getSleepMode() != WIFI_LIGHT_SLEEP)
    {
        if (debugMode)
        {
            debugPrint("Back to light sleep mode");
        }
        if (!WiFi.setSleepMode(WIFI_LIGHT_SLEEP))
        {
            if (debugMode)
            {
                debugPrint("Failed to set light sleep mode");
            }
        }
    }
    /***** Time update loop *****/
    // As long as we keep receiving time from IU server, this function will do
    // nothing (see timeManager.TIME_UPDATE_INTERVAL for max delay before time 
    // update from NTP server)
    timeManager.updateTimeReferenceFromNTP();
    /***** MQTT Connection loop *****/
    iuMQTTHelper.loop(DIAGNOSTIC_TOPIC, WILL_MESSAGE, onMQTTConnection,
                      15000);
    /***** Send wifi status *****/
    if (iuWifiManager.isTimeToSendWifiInfo())
    {
        char message[256];
        iuWifiManager.getWifiInfo(message);
        publishDiagnostic(message, strlen(message));
    }
    /***** Read message from main board and process them *****/
    readAllMessagesFromHost();
    // Second call to publishAccelRawDataIfReady in case first request failed
    publishAccelRawDataIfReady();
//    if (AUTHORIZED_TO_SLEEP)
//    {
//        Serial.println("Sleep...");
//        delay(1000);
//    }
}
