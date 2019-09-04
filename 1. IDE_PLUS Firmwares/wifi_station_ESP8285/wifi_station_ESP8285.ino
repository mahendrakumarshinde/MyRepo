/*
  Infinite Uptime BLE Module Firmware

  Update 2018-03-01
*/

#include "Conductor.h"
#include <Ticker.h>
#include <rom/rtc.h>

uint32_t lastprint = 0;

Conductor conductor;

/* =============================================================================
    MQTT callbacks
============================================================================= */

/**
 * Callback called when a new MQTT message is received from MQTT server.
 *
 * See conductor.processMessageFromMQTT
 */
void mqttNewMessageCallback(char* topic, byte* payload, unsigned int length) // ESP32_PORT_TRUE
{
    conductor.processMessageFromMQTT(topic, (char*) payload, length);
}

/**
 * Callback for MQTT (re)connections.
 */
void onMQTTConnection()
{
    mqttHelper.onConnection();
    conductor.publishDiagnostic("connected", 10);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_FIRMWARE_VERSION);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_SAMPLING_RATE);
    hostSerial.sendMSPCommand(MSPCommand::ASK_HOST_BLOCK_SIZE);
}

/* =============================================================================
    Host message callback
============================================================================= */

void onNewHostMessageFromHost(IUSerial *iuSerial) {
    conductor.processHostMessage(iuSerial);
}

/* =============================================================================
    Main setup and loop
============================================================================= */

void setup()
{
    char TestStr1[64];    
    sprintf(TestStr1,"Reset:%d C0:%d C1:%d",esp_reset_reason(),(int)rtc_get_reset_reason(0),(int)rtc_get_reset_reason(1));
    hostSerial.begin();
    hostSerial.setOnNewMessageCallback(onNewHostMessageFromHost);
    if (debugMode) {
        delay(5000);
    }
    hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, TestStr1,32);
    #if IUDEBUG_ANY == 1
        conductor.forceWiFiConfig(testSSID, testPSK, testStaticIP,
                                  testGateway, testSubnet);
        conductor.setBleMAC(hostMacAddress);
    #else
        conductor.getConfigFromMainBoard();
    #endif
    // If this point is reached, tell host that WiFi is waking up
    hostSerial.sendMSPCommand(MSPCommand::WIFI_ALERT_AWAKE);
    // Prepare to receive MQTT messages
    mqttHelper.client.setCallback(mqttNewMessageCallback);
    mqttHelper.setOnConnectionCallback(onMQTTConnection);
    delay(100);
    #if IUDEBUG_ANY == 1
        conductor.reconnect(true);
    #endif
    hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, "@ WIFI_CLIENT SETP @",20);
}

/**
 * Unless there is a connection attempt going on, the loop typically takes
 * ~ 1ms to complete, without accounting for the delay at the end.
 * The delay is there to allow the WiFi to light-sleep.
 */
void loop()
{
    hostSerial.readMessages();  // Read and process messages from host
    if (conductor.reconnect()) {  // If Wifi is connected
        if (!timeHelper.active()) {
            timeHelper.begin();
        }
        // Update time reference from NTP server if not yet received
        // from IU server
        timeHelper.updateTimeReferenceFromNTP();
        /***** MQTT Connection / message reception loop *****/
        conductor.loopMQTT();
        conductor.publishWifiInfoCycle();
        // Publish raw data (HTTP POST request)
//        accelRawDataHelper.publishIfReady(conductor.getBleMAC());
        
    }
#if 0
    if(millis() - lastprint > 7000)
    {
        lastprint = millis();
        hostSerial.sendMSPCommand(MSPCommand::ESP_DEBUG_TO_STM_HOST, "WiFi Loop OK",12);
    }
#endif    
    conductor.updateWiFiStatusCycle();
    conductor.checkWiFiDisconnectionTimeout();
    delay(1);
}
