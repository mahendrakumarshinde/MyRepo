/*
  Infinite Uptime WiFi Module Firmware
  Update 12-03-2020
*/

#include "Conductor.h"
#include <Ticker.h>
#include <rom/rtc.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
//#include "IUESPFlash.h"
//#include "SPIFFS.h"
Conductor conductor;
uint32_t lastDone = 0;
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

void getAllConfig()
{   
    mqttHelper.onConnection();
    conductor.publishDiagnostic("connected", 10);
    hostSerial.sendMSPCommand(MSPCommand::GET_DEVICE_CONFIG);
    delay(10);
    hostSerial.sendMSPCommand(MSPCommand::ASK_WIFI_CONFIG);
    
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
    hostSerial.begin();
    hostSerial.setOnNewMessageCallback(onNewHostMessageFromHost);
    if (debugMode) {
        delay(5000);
    }
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
    // mqttHelper.setOnConnectionCallback(onMQTTConnection);
    mqttHelper.setOnConnectionCallback(getAllConfig);           // Once the MQTT client connected 
    delay(100);
    #if IUDEBUG_ANY == 1
        conductor.reconnect(true);
    #endif

     
    iuWiFiFlash.begin();
    // Set the common url json if file not present
    conductor.setCommonHttpEndpoint();
    //Configure the Diagnostic HTTP/HTTPS Endpoint
    conductor.configureDiagnosticEndpointFromFlash(IUESPFlash::CFG_DIAGNOSTIC_ENDPOINT);
    conductor.activeCertificates = iuWiFiFlash.readMemory(ADDRESS);

   
    conductor.connectToWiFi();
    
}

/**
 * Unless there is a connection attempt going on, the loop typically takes
 * ~ 1ms to complete, without accounting for the delay at the end.
 * The delay is there to allow the WiFi to light-sleep.
 */
void loop()
{
    if(!conductor.configStatus){
        getAllConfig();
    }
    hostSerial.readMessages();  // Read and process messages from host
    if (conductor.reconnect()) {  // If Wifi is connected
        if (!timeHelper.active()) {
            timeHelper.begin();
        }
        // Update time reference from NTP server if not yet received
        // from IU server
        timeHelper.updateTimeReferenceFromNTP();
        /***** MQTT Connection / message reception loop *****/
        // if(!mqttHelper.client.connected()){
        //      conductor.loopMQTT();
        // }
        conductor.publishWifiInfoCycle();
        // Publish raw data (HTTP POST request)
//        accelRawDataHelper.publishIfReady(conductor.getBleMAC());
        
    }
    conductor.mqttSecureConnect();
    conductor.updateWiFiStatusCycle();
    conductor.checkWiFiDisconnectionTimeout();
    conductor.checkMqttDisconnectionTimeout();
    conductor.checkOtaPacketTimeout();
    if(WiFi.isConnected() == false)         // Need Auto reconnect for MQTT broker
    {   
        conductor.autoReconncetWifi();
    } 
    uint32_t now = millis();
    static uint8_t rssiPublishedCounter;
    if (now - lastDone > 5000 )
    {   
        //iuWiFiFlash.listAllAvailableFiles(IUESPFlash::CONFIG_SUBDIR);
        Serial.print("EEPROM Value :");
        Serial.println(iuWiFiFlash.readMemory(ADDRESS));

        rssiPublishedCounter++ ;
        if (rssiPublishedCounter >= 6)
        {
            conductor.publishRSSI();
            rssiPublishedCounter = 0;
        }
        conductor.resetDownloadInitTimer(60,5000);
        lastDone = now;
        if(uint64_t(conductor.getBleMAC() ) == 0) { 
            hostSerial.sendMSPCommand(MSPCommand::ASK_BLE_MAC);
            delay(10);
            hostSerial.sendMSPCommand(MSPCommand::GET_MQTT_CONNECTION_INFO);
            delay(10);
            hostSerial.sendMSPCommand(MSPCommand::GET_RAW_DATA_ENDPOINT_INFO); 
        }
    }
    delay(1);
}
