/*
  Infinite Uptime BLE Module Firmware

  Update 2017-12-06
*/

#include "Conductor.h"

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


/* =============================================================================
    MQTT callbacks
============================================================================= */

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * See conductor.processMessageFromMQTT
 */
void mqttNewMessageCallback(char* topic, byte* payload, uint16_t length)
{
    conductor.processMessageFromMQTT(topic, payload, length);
}

/**
 * Callback for MQTT (re)connections.
 */
void onMQTTConnection()
{
    iuMQTTHelper.onConnection();
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

