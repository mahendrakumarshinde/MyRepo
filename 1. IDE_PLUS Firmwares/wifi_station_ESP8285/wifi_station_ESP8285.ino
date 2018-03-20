/*
  Infinite Uptime BLE Module Firmware

  Update 2018-03-01
*/

#include "Conductor.h"
#include <Ticker.h>


Conductor conductor;

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
    mqttHelper.onConnection(timeHelper.getCurrentTime());
}


/* =============================================================================
    Status Updater
============================================================================= */

Ticker cloudStatusUpdater;

void sendWiFiInfoToCloud()
{
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        char message[256];
        conductor.getWifiInfo(message, true);
        mqttHelper.publishDiagnostic(message, strlen(message),
                                     timeHelper.getCurrentTime());
    }
}

Ticker hostConnectionStatusUpdater;

void sendConnectionStatusToHost()
{
    if (WiFi.isConnected() && mqttHelper.client.connected())
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_CONNECTED);
    }
    else
    {
        hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_DISCONNECTED);
    }
}


/* =============================================================================
    Main setup and loop
============================================================================= */

void setup()
{
    hostSerial.begin();
    // Turn off radio at wake up to save power
    conductor.turnOffRadio();
    // Get config: should the ESP sleep? What's the BLE MAC address?
    #ifdef IUDEBUG_ANY
        conductor.setCredentials(testSSID, testPSK);
    #else
        conductor.getConfigFromMainBoard();
    #endif
    // If this point is reached, tell host that WiFi is waking up
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_AWAKE);
    // Prepare to receive MQTT messages
    mqttHelper.client.setCallback(mqttNewMessageCallback);
    mqttHelper.setOnConnectionCallback(onMQTTConnection);
    // Configure WiFi and turn radio back on
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    conductor.turnOnRadio();
    // Attach the timers
    cloudStatusUpdater.attach(300, sendWiFiInfoToCloud);
    hostConnectionStatusUpdater.attach(5, sendConnectionStatusToHost);
    delay(100);
}

void loop()
{
    conductor.readMessagesFromHost();  // Read and process messages from host
    if (conductor.reconnect())  // If Wifi is connected
    {
        if (!timeHelper.active())
        {
            timeHelper.begin();
        }
        // Update time reference from NTP server if not yet received
        // from IU server
        timeHelper.updateTimeReferenceFromNTP();
        /***** MQTT Connection / message reception loop *****/
        mqttHelper.loop();
        // Publish raw data (HTTP POST request)
        accelRawDataHelper.publishIfReady(conductor.getBleMacAddress());
    }
    conductor.checkWiFiDisconnectionTimeout();
    // Light sleep (but keep listening to serial)
    uint32_t sleepEnd = millis() + 1000;
    while (millis() < sleepEnd && Serial.available() == 0)
    {
        delay(10);
    }
}


